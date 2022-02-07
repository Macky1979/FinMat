#include <string>
#include <iostream>
#include <tuple>
#include <map>
#include <vector>
#include <memory>
#include "lib_date.h"
#include "lib_dataframe.h"
#include "fin_curve.h"
#include "fin_fx.h"
#include "fin_date.h"
#include "fin_cap_floor.h"
#include "lib_aux.h"

/*
 * AUXILIARY FUNCTIONS
 */

// extract vector of dates from vector of events
static std::vector<myDate> extract_dates_from_events(const std::vector<cap_flr_event> &events, const std::string &type)
{
    // create pointer to vector of dates
    std::vector<myDate> dates;
 
    // reserve memory
    dates.reserve(events.size());

    // for event by event and extract the date
    for (int idx = 0; idx < events.size(); idx++)
    {
        if (type.compare("date_begin") == 0)
        {
            dates.emplace_back(events[idx].date_begin);
        }
        else if (type.compare("date_end") == 0)
        {
            dates.emplace_back(events[idx].date_end);
        }
        else
        {
            throw std::runtime_error((std::string)__func__ + ": " + type + " is not supported type!");
        }
    }

    // return the vector with extracted dates
    return dates;
}

/*
 * OBJECT CONSTRUCTORS
 */

myCapsFloors::myCapsFloors(const mySQLite &db, const std::string &sql, const myDate &calc_date)
{
    // dataframe to hold result of SQL query
    myDataFrame * rslt = new myDataFrame();

    // load bond portfolio as specified by SQL query
    rslt = db.query(sql);

    // auxiliary variables
    std::string aux;
    myDate date_aux;
    std::vector<myDate> event_dates;
    std::vector<myDate> begin_cpn_dates;
    std::vector<myDate> end_cpn_dates;

    // reserve memory to avoid memory resize
    this->info.reserve(rslt->tbl.values.size());

    // go bond by bond
    for (int cap_flr_idx = 0; cap_flr_idx < rslt->tbl.values.size(); cap_flr_idx++)
    {
        // load bond information and perform some sanity checks
        cap_flr_info cap_flr;

            // entity name
            cap_flr.ent_nm = rslt->tbl.values[cap_flr_idx][0];

            // parent id; useful to bind two or more bonds together to create a new product like IRS
            cap_flr.parent_id = rslt->tbl.values[cap_flr_idx][1];

            // contract id
            cap_flr.contract_id = rslt->tbl.values[cap_flr_idx][2];

            // issuer id
            cap_flr.issuer_id = rslt->tbl.values[cap_flr_idx][3];

            // portfolio
            cap_flr.ptf = rslt->tbl.values[cap_flr_idx][4];

            // account
            cap_flr.account = rslt->tbl.values[cap_flr_idx][5];

            // bond ISIN
            cap_flr.isin = rslt->tbl.values[cap_flr_idx][6];

            // comments
            cap_flr.comments = rslt->tbl.values[cap_flr_idx][7];

            // fixing type - "fwd" and "par" for a floating bond vs.
            // "fix" for a fixed bond
            aux = rslt->tbl.values[cap_flr_idx][9];
            if ((aux.compare("fwd") != 0) && (aux.compare("par") != 0) && (aux.compare("fix") != 0))
            {
                cap_flr.wrn_msg += "unsupported fixing type " + aux + ";";
                cap_flr.fix_type = "fix";
            }

            if (aux.compare("fix") == 0)
            {
                cap_flr.is_fixed = true;
            }
            else
            {
                cap_flr.is_fixed = false;
            }
            cap_flr.fix_type = aux;

            // bond rating
            cap_flr.rtg = rslt->tbl.values[cap_flr_idx][10];

            // bond currency, e.g. EUR, CZK
            cap_flr.ccy_nm = rslt->tbl.values[cap_flr_idx][11];

            // bond nominal
            cap_flr.nominal = stod(rslt->tbl.values[cap_flr_idx][12]);

            // bond deal date, i.e. date when the bond entered / will enter into books
            cap_flr.deal_date = myDate(stoi(rslt->tbl.values[cap_flr_idx][13]));

            // bond maturity date
            cap_flr.maturity_date = myDate(stoi(rslt->tbl.values[cap_flr_idx][14]));

            // day count method used to calculate coupon payment
            cap_flr.dcm = rslt->tbl.values[cap_flr_idx][15];

            // bond accrued interest
            aux = rslt->tbl.values[cap_flr_idx][16];
            if (aux.compare("") == 0) // accrued interest not provided
            {
                cap_flr.wrn_msg += "accrued interest not provided;";
                cap_flr.is_acc_int = false;
                cap_flr.acc_int = 0.0;
            }
            else // accrued interest provided
            {
                cap_flr.is_acc_int = true;
                cap_flr.acc_int = stod(aux);
            }

            // coupon rate
            aux = rslt->tbl.values[cap_flr_idx][17];
            if (aux.compare("") == 0) // coupon rate not provided
            {
                cap_flr.wrn_msg += "coupon rate not provided;";
                cap_flr.is_fixed = true;
                cap_flr.is_acc_int = true;
                cap_flr.acc_int = 0.0;
                cap_flr.cpn_rate = 0.0;
                cap_flr.cpn_freq = "";
                cap_flr.fix_type = "fix";
                cap_flr.first_cpn_date = cap_flr.maturity_date;
            }
            else // coupon rate provided
            {
                cap_flr.cpn_rate = stod(aux);
            }
             
            // first coupon date
            aux = rslt->tbl.values[cap_flr_idx][18];
            if (aux.compare("") == 0) // first coupon date not provided
            {
                cap_flr.wrn_msg += "first coupon date not provided;";
                cap_flr.is_fixed = true;
                cap_flr.is_acc_int = true;
                cap_flr.acc_int = 0.0;
                cap_flr.cpn_rate = 0.0;
                cap_flr.cpn_freq = "";
                cap_flr.fix_type = "fix";
                cap_flr.first_cpn_date = cap_flr.maturity_date;
            }
            else // the first coupon date provided
            {
                cap_flr.first_cpn_date = myDate(stoi(aux));
            }

            // coupon frequency
            aux = rslt->tbl.values[cap_flr_idx][19];
            if ((aux.compare("") == 0) && cap_flr.cpn_rate != 0.0) // coupon frequency not provided for non-zero coupon rate
            {
                cap_flr.wrn_msg += "coupon frequency not provided for coupon bearing bond;";
                cap_flr.is_fixed = true;
                cap_flr.is_acc_int = true;
                cap_flr.acc_int = 0.0;
                cap_flr.cpn_rate = 0.0;
                cap_flr.cpn_freq = "";
                cap_flr.fix_type = "fix";
                cap_flr.first_cpn_date = cap_flr.maturity_date;
            }
            else if ((aux.compare("") != 0) && (cap_flr.cpn_rate == 0)) // copoun frequency provided for zero coupon rate
            {
                cap_flr.wrn_msg += "coupon frequency provided for zero coupon bond;";
                cap_flr.is_fixed = true;
                cap_flr.is_acc_int = true;
                cap_flr.acc_int = 0.0;
                cap_flr.cpn_rate = 0.0;
                cap_flr.cpn_freq = "";
                cap_flr.fix_type = "fix";
                cap_flr.first_cpn_date = cap_flr.maturity_date;
            }
            else // coupon frequency provided for non-zero coupon rate
            {
                cap_flr.cpn_freq = aux;
            }

            // first fixing date of a floating bond
            aux = rslt->tbl.values[cap_flr_idx][20];
            if (aux.compare("") == 0) // fixing date not provided
            {
                if (!cap_flr.is_fixed) // floating bond
                {
                    cap_flr.wrn_msg += "first fixing date not provided for a floating bond;";
                    cap_flr.is_fixed = true;
                    cap_flr.fix_freq = "";
                    cap_flr.fix_type = "fix";
                }
            }
            else // fixing date provided
            {
                if (!cap_flr.is_fixed) // floating bond
                {
                    cap_flr.first_fix_date = myDate(stoi(aux));
                }
                else // fixed bond
                {
                    cap_flr.wrn_msg += "first fixing date provided for a fixed bond;";
                }
            }
            
            // fixing frequency
            aux = rslt->tbl.values[cap_flr_idx][21];
            if (aux.compare("") == 0) // fixing frequency not provided
            {
                if (!cap_flr.is_fixed) // floating bond
                {
                    cap_flr.wrn_msg += "fixing frequency not provided a floating bond";
                    cap_flr.is_fixed = true;
                    cap_flr.fix_freq = "";
                    cap_flr.fix_type = "fix";
                }
                else // fixed bond
                {
                    cap_flr.fix_freq = "";
                }
            }
            else // fixing frequency provided
            {
                if (!cap_flr.is_fixed) // floating bond
                {
                    cap_flr.fix_freq = aux;
                }
                else // fixed bond
                {
                    cap_flr.wrn_msg += "fixing frequency provided for a fixed bond;";
                    cap_flr.fix_freq = "";
                }
            }

            // amortization
            std::string first_amort_date = rslt->tbl.values[cap_flr_idx][22];
            std::string amort_freq = rslt->tbl.values[cap_flr_idx][23];
            std::string amort = rslt->tbl.values[cap_flr_idx][24];
            
            if (amort.compare("") == 0) // amortization amount not provided
            {
                cap_flr.amort = 0.0;

                if (amort_freq.compare("") != 0) // amortization frequency provided
                {
                    cap_flr.wrn_msg += "amortization frequency provided for a bond with zero amortization amount;";
                }
                cap_flr.amort_freq = "";

                if (first_amort_date.compare("") != 0) // amortization frequency provided 
                {
                    cap_flr.wrn_msg += "first amortization date provided for a bond with zero amortization amount;";
                }
            }
            else // amortization amount provided
            {
                cap_flr.amort = stod(amort);

                if (amort_freq.compare("") == 0) // amortization frequency not provided
                {
                    cap_flr.wrn_msg += "amortization frequency not provided for a bond with non-zero amortization amount;";
                    cap_flr.amort = 0.0;
                    cap_flr.amort_freq = "";
                }
                else // amortization frequency provided
                {
                    cap_flr.amort_freq = amort_freq;
                }

                if (first_amort_date.compare("") == 0) // first amortization date not provided
                {
                    cap_flr.wrn_msg += "first amortization date not provided for a bond with non-zero amortization amount;";
                    cap_flr.amort = 0.0;
                    cap_flr.amort_freq = "";
                }
                else // first amortization date provided
                {
                    cap_flr.first_amort_date = myDate(stoi(first_amort_date));
                }
            }
            
            // discounting curve
            cap_flr.crv_disc = rslt->tbl.values[cap_flr_idx][27];

            // repricing curve
            aux = rslt->tbl.values[cap_flr_idx][28];
            if (aux.compare("") == 0) // repricing curve not specified
            {
                if (!cap_flr.is_fixed) // floating bond
                {
                    cap_flr.wrn_msg += "repricing curve not provided for a floating bond;";
                    cap_flr.is_fixed = true;
                    cap_flr.fix_freq = "";
                    cap_flr.fix_type = "fix";
                }
                cap_flr.crv_fwd = "";
            }
            else // repricing curve specified
            {
                if (!cap_flr.is_fixed) // floating bond
                {
                    cap_flr.crv_fwd = aux;
                }
                else // fixed bond
                {
                    cap_flr.wrn_msg += "repricing curve specified for a fixed bond;";
                    cap_flr.crv_fwd = "";
                }
            }

        // perform other sanity checks
       
            // deal date
            if (cap_flr.deal_date.get_days_no() > cap_flr.maturity_date.get_days_no())
            {
                cap_flr.wrn_msg += "deal date cannot by greater than maturity date;";
                cap_flr.deal_date = cap_flr.maturity_date;
            }

            // first coupon date
            if (cap_flr.first_cpn_date.get_days_no() <= cap_flr.deal_date.get_days_no())
            {
                cap_flr.wrn_msg += "first coupon date cannot be lower or equal to deal date;";
                cap_flr.first_cpn_date = cap_flr.deal_date;
                cap_flr.first_cpn_date.add(cap_flr.cpn_freq);
                while (cap_flr.first_cpn_date.get_days_no() < cap_flr.deal_date.get_days_no())
                {
                    cap_flr.first_cpn_date.add(cap_flr.cpn_freq);
                }
            }

            if (cap_flr.first_cpn_date.get_days_no() > cap_flr.maturity_date.get_days_no())
            {
                cap_flr.wrn_msg += "first coupon date cannot be greater than maturity date;";
                cap_flr.first_cpn_date = cap_flr.maturity_date;
            }

            // first fixing date
            if (!cap_flr.is_fixed)
            {
                if (cap_flr.first_fix_date.get_days_no() <= cap_flr.deal_date.get_days_no())
                {
                    cap_flr.wrn_msg += "first fixing date cannot be lower or equal to deal date;";
                    cap_flr.first_fix_date = cap_flr.first_cpn_date;
                }

                if (cap_flr.first_fix_date.get_days_no() > cap_flr.maturity_date.get_days_no())
                {
                    cap_flr.wrn_msg += "first fixing date cannot be greater than maturity date;";
                    cap_flr.first_fix_date = cap_flr.first_cpn_date;
                }

                if (cap_flr.first_fix_date.get_days_no() <= cap_flr.first_cpn_date.get_days_no())
                {
                    cap_flr.wrn_msg += "first fixing date cannot be lower or equal to the first copoun date;";
                    cap_flr.first_fix_date = cap_flr.first_cpn_date;
                }
            }

            // first amortization date
            if (cap_flr.amort != 0)
            {
                if (cap_flr.first_amort_date.get_days_no() <= cap_flr.deal_date.get_days_no())
                {
                    cap_flr.wrn_msg += "first amortization date cannot be lower or equal to deal date;";
                    cap_flr.first_amort_date = cap_flr.first_cpn_date;
                }

                if (cap_flr.first_amort_date.get_days_no() > cap_flr.maturity_date.get_days_no())
                {
                    cap_flr.wrn_msg += "first amortization date cannot be greater than maturity date;";
                    cap_flr.first_amort_date = cap_flr.first_cpn_date;
                }

                if (cap_flr.first_amort_date.get_days_no() <= cap_flr.first_cpn_date.get_days_no())
                {
                    cap_flr.wrn_msg += "first amortization date cannot be lower or equal to the first copoun date;";
                    cap_flr.first_amort_date = cap_flr.first_cpn_date;
                }
            }

        // check frequencies
        
            // amortization frequency
            if (cap_flr.amort_freq.compare("") != 0)
            {
                if (eval_freq(cap_flr.amort_freq) < eval_freq(cap_flr.cpn_freq))
                {
                    cap_flr.wrn_msg += "amortization frequency cannot be lower than coupon frequency;";
                    cap_flr.amort_freq = cap_flr.cpn_freq;
                }
            }

            // repricing frequency
            if (cap_flr.fix_freq.compare("") != 0)
            {
                if (eval_freq(cap_flr.fix_freq) < eval_freq(cap_flr.cpn_freq))
                {
                    cap_flr.wrn_msg += "repricing frequency cannot be lower than coupon frequency;";
                    cap_flr.fix_freq = cap_flr.cpn_freq;
                }
            }

        // generate vector of events

            // variables containing dates
            myDate date1;
            myDate date2;

            // events
            cap_flr_event evnt;
            std::vector<cap_flr_event> events;

            // position index
            int pos_idx;

            // date format, which is used to store date string in object myDate
            std::string date_format = "yyyymmdd";

            // find date of the first copoun payment and a create vector coupon events;
            // we assume that all other events occur on coupon payment dates
            if (cap_flr.cpn_freq.compare("") != 0) // coupon bond
            {
                // iterate until you find the first coupon period that does not preceed calculation date
                date1 = cap_flr.first_cpn_date;
                date2 = cap_flr.first_cpn_date;
                date2.remove(cap_flr.cpn_freq);
                while ((date1.get_days_no() < calc_date.get_days_no()) & (date2.get_days_no() < calc_date.get_days_no()))
                {
                    date1 = date2;
                    date2.add(cap_flr.cpn_freq);
                }
                
                // date1 represents the beginning date of such coupon period => we generate coupon dates till maturity
                event_dates = create_date_serie(date1.get_date_str(), cap_flr.maturity_date.get_date_str(), cap_flr.cpn_freq, date_format);     

                // create a vector of events based on coupon dates
                for (int idx = 0; idx < event_dates.size(); idx++)
                {
                    date1 = event_dates[idx];
                    date2 = date1;
                    date1.remove(cap_flr.cpn_freq);
                    evnt.date_begin = date1; // beging of coupon period
                    evnt.date_end = date2; // end of coupon period => date of coupon payment
                    evnt.cpn_year_frac = day_count_method(date1, date2, cap_flr.dcm);
                    evnt.is_cpn_payment = true;

                    // indicated that coupon payment is fixed and therefore
                    // could be calculated without a scenario knowledge
                    if (cap_flr.is_fixed)
                    {
                        evnt.is_cpn_fixed = true;
                    }
                    else
                    {
                        // even a floating bond has its first coupon payment
                        // fixed if it is not a forward contract
                        if (evnt.date_begin.get_days_no() < cap_flr.first_fix_date.get_days_no())
                        {
                            evnt.is_cpn_fixed = true;
                        }
                        // coupon payments in fugure
                        else
                        {
                            evnt.is_cpn_fixed = false;
                        }
                    }

                    events.emplace_back(evnt);
                }
            }
            else // zero coupon bond
            {
                evnt.date_begin = cap_flr.maturity_date;
                evnt.date_end = cap_flr.maturity_date;
                evnt.cpn_year_frac = 0.0;
                evnt.is_cpn_payment = true;
                evnt.is_cpn_fixed = true;
                events.emplace_back(evnt);
            }

            // extract begin and end dates of the coupon periods
            begin_cpn_dates = extract_dates_from_events(events, "date_begin");
            end_cpn_dates = extract_dates_from_events(events, "date_end");

            // add amortization information to events vector
            if (cap_flr.amort_freq.compare("") != 0)
            {
                // iterate until you find the first amortization date after calculation date
                date1 = cap_flr.first_amort_date;
                while (date1.get_days_no() < calc_date.get_days_no())
                {
                    date1.add(cap_flr.amort_freq);
                }

                // date1 is the first amortization date => we generate amortizaton dates till maturity
                event_dates = create_date_serie(date1.get_date_str(), cap_flr.maturity_date.get_date_str(), cap_flr.amort_freq, date_format);

                // go amortization date by amortization date and assign it the nearest coupon date; we assume that amortizations happen on coupon dates
                for (int idx = 0; idx < event_dates.size(); idx++)
                {
                    // we assign amortization flag based on begining coupon date
                    if (event_dates[idx].get_date_int() < cap_flr.maturity_date.get_date_int())
                    {
                        pos_idx = get_nearest_idx(event_dates[idx], end_cpn_dates, "abs");
                        events[pos_idx].amort_flg = true;
                    }
                }
            }

            // add nominal information; we assume that cap_flr.nominal contains the current bond nominal (i.e. taking potential previous amortizations
            // into account)
            for (int idx = 0; idx < events.size(); idx++)
            {
                // set amortization payment to zero by default
                events[idx].amort_payment = 0.0;

                // set nominal
                if (idx == 0) // first coupon date => use the current bond nominal
                {
                    events[idx].nominal_begin = cap_flr.nominal;
                    events[idx].nominal_end = cap_flr.nominal;
                }
                else // on the following coupon dates we use nominal from the previous coupon date
                {
                    events[idx].nominal_begin = events[idx - 1].nominal_end;
                    events[idx].nominal_end = events[idx - 1].nominal_end;
                }

                // adjust for amortization payment
                if (events[idx].amort_flg)
                {
                    events[idx].nominal_end -= cap_flr.amort;
                    events[idx].amort_payment = cap_flr.amort;
                }

                // the last coupon payment is accompanied by full nominal repayment
                if (idx == events.size() - 1)
                {
                    events[idx].amort_payment = events[idx].nominal_begin;
                    events[idx].nominal_end = 0.0;
                }
            }

            // add repricing information
            if (cap_flr.fix_freq.compare("") != 0)
            {
                // iterate until you find the first repricing date after calculation date
                date1 = cap_flr.first_fix_date;
                while (date1.get_days_no() < calc_date.get_days_no())
                {
                    date1.add(cap_flr.fix_freq);
                }

                // date1 is the first repricing date => we generate repricing dates till maturity
                event_dates = create_date_serie(date1.get_date_str(), cap_flr.maturity_date.get_date_str(), cap_flr.fix_freq, date_format);

                // determine in which coupon periods there will be repricing
                for (int idx = 0; idx < event_dates.size(); idx++)
                {
                    // we assign repricing flag based on begining coupon date
                    if (event_dates[idx].get_date_int() < cap_flr.maturity_date.get_date_int())
                    {
                        pos_idx = get_nearest_idx(event_dates[idx], begin_cpn_dates, "abs");
                        events[pos_idx].fix_flg = true;
                    }
                }

                // repricing dates
                if (cap_flr.fix_type.compare("fix") != 0)
                {
                    for (int idx = 0; idx < events.size(); idx++)
                    {
                        // coupon rate was fixed in the previous coupon period; could occur if
                        // e.g coupon period is 6M and repricing period is 1Y
                        if (!events[idx].is_cpn_fixed && !events[idx].fix_flg)
                        {
                            events[idx].cpn = events[idx - 1].cpn;
                        }
                        // determine coupon rate
                        else if (!events[idx].is_cpn_fixed && events[idx].fix_flg)
                        {
                            date1 = events[idx].date_begin;
                            date2 = date1;
                            date2.add(cap_flr.fix_freq);
                       
                            // forward rate repricing - only the boundary dates
                            if (cap_flr.fix_type.compare("fwd") == 0)
                            {
                                events[idx].repricing_dates.emplace_back(date1);
                                events[idx].repricing_dates.emplace_back(date2);
                            }

                            // par rate repricing - not only boundary repricing
                            // dates but also in between coupon dates
                            else if (cap_flr.fix_type.compare("par") == 0)
                            {
                                for (int idx2 = 0; idx2 < events.size(); idx2++)
                                {
                                    // check that coupon date falls within
                                    // the boundary repricing dates
                                    if ((events[idx2].date_end.get_days_no() >= date1.get_days_no()) &&
                                        (events[idx2].date_end.get_days_no() <= date2.get_days_no()))
                                    {
                                        events[idx].repricing_dates.emplace_back(events[idx2].date_end);
                                        events[idx].par_nominals_begin.emplace_back(events[idx2].nominal_begin);
                                        events[idx].par_nominals_end.emplace_back(events[idx2].nominal_end);
                                    }
                                }
                           }
                        }
                    }
                }
            }

            // calculate fixed cupoun payments and accrued interest
            for (int idx = 0; idx < events.size(); idx++)
            {
                if (events[idx].is_cpn_fixed)
                {
                    // calculate accrued interest for the first coupon payment
                    // if neccessary
                    if (!cap_flr.is_acc_int && (idx == 0))
                    {
                        if (events[idx].date_begin.get_days_no() <= calc_date.get_days_no())
                        {
                            // year fraction for the first coupon payment
                            double acc_int_year_frac = day_count_method(events[idx].date_begin, calc_date, cap_flr.dcm);
                            cap_flr.acc_int = cap_flr.cpn_rate * events[idx].nominal_begin * acc_int_year_frac;
                        }
                        // in case of a forward contract accrued interest is zero
                        else
                        {
                            cap_flr.acc_int = 0.0;
                        }
                    }
                }
            }
        
            // add events into bond structure
            cap_flr.events = events;

            // add bond to vector of bonds
            this->info.emplace_back(cap_flr);
    
        }

    // delete unused pointers
    delete rslt;
}

/*
 * OBJECT FUNCTIONS
 */

// split the object
std::vector<myCapsFloors> myCapsFloors::split(const int &threads_no)
{

    // determine position indicies of the newly created vectors
    std::vector<coordinates<int>> indicies = split_vector(this->info.size(), threads_no);

    // split original object myAnnuities into several smaller objects
    std::vector<myCapsFloors> caps_flrs;
    for (int thread_idx = 0; thread_idx < threads_no; thread_idx++)
    {
        // determine the first and the last position index
        std::vector<cap_flr_info>::const_iterator first = this->info.begin() + indicies[thread_idx].x;
        std::vector<cap_flr_info>::const_iterator last = this->info.begin() + indicies[thread_idx].y + 1;

        // extract bond information
        std::vector<cap_flr_info> info(first, last);

        // create new smaller object based on the position indicies
        myCapsFloors cap_flr(info);

        // add the new object to the vector
        caps_flrs.emplace_back(cap_flr);
    }

    // release memory by clearing the original object
    this->clear();

    // return vector of objects
    return caps_flrs;
}

// merge several objects
void myCapsFloors::merge(std::vector<myCapsFloors> &caps_flrs)
{
    // pre-allocate memory
    int final_vector_size = 0;
    std::vector<cap_flr_info> info;
    
    for (int idx = 0; idx < caps_flrs.size(); idx++)
    {
        final_vector_size += caps_flrs[idx].info.size();
    }

    info.reserve(final_vector_size);

    // merge individual vectors
    for (int idx = 0; idx < caps_flrs.size(); idx++)
    {
        info.insert(info.end(), caps_flrs[idx].info.begin(), caps_flrs[idx].info.end());
        caps_flrs[idx].clear(); // release memory by clearing individual objects
    }

    // copy merged vectors into the current object
    this->info = info;
}

// calculate NPV
void myCapsFloors::calc_npv(const int &scn_no, const myCurves &crvs, const myFx &fx, const std::string &ref_ccy_nm)
{
    // variables 
    std::vector<std::tuple<int, int>> scn_tenors;

    // go bond by bond
    for (int cap_flr_idx = 0; cap_flr_idx < this->info.size(); cap_flr_idx++)
    {
        // calculate repricing rates for floating bonds
        if (!this->info[cap_flr_idx].is_fixed)
        {
            // go event by event
            for (int idx = 0; idx < this->info[cap_flr_idx].events.size(); idx++)
            {
                // coupon is already fixed
                if (this->info[cap_flr_idx].events[idx].is_cpn_fixed | !this->info[cap_flr_idx].events[idx].fix_flg)
                {
                    if (idx > 0)
                    {
                        this->info[cap_flr_idx].events[idx].cpn = this->info[cap_flr_idx].events[idx - 1].cpn;
                    }
                }
                // calculate forward rate / par rate for unfixed coupon rates
                else
                {
                    // prepare vector with scenario number and tenors
                    scn_tenors.clear();
                    for (int idx2 = 0; idx2 < this->info[cap_flr_idx].events[idx].repricing_dates.size(); idx2++)
                    {
                        scn_tenors.push_back(std::tuple<int, int>(scn_no, this->info[cap_flr_idx].events[idx].repricing_dates[idx2].get_date_int()));
                    }

                    // apply forward rate
                    if (this->info[cap_flr_idx].fix_type.compare("fwd") == 0)
                    {
                        // get forward rate
                        std::vector<double> fwds = crvs.get_fwd_rate(this->info[cap_flr_idx].crv_fwd, scn_tenors, this->info[cap_flr_idx].dcm);

                        // use the forward rate as a coupon rate
                        this->info[cap_flr_idx].events[idx].cpn = fwds[0];
                    }
                    // apply par-rate
                    else
                    {
                        // get par-rate
                        int par_step = this->info[cap_flr_idx].events[idx].repricing_dates.size() - 1;
                        std::vector<double> pars = crvs.get_par_rate(this->info[cap_flr_idx].crv_fwd, scn_tenors, this->info[cap_flr_idx].events[idx].par_nominals_begin, this->info[cap_flr_idx].events[idx].par_nominals_end, par_step, this->info[cap_flr_idx].dcm);

                        // user the par-rate as a coupon rate
                        this->info[cap_flr_idx].events[idx].cpn = pars[0];
                    }
                }
            }
        }

        // calculate discount factors and total bond NPV
        for (int idx = 0; idx < this->info[cap_flr_idx].events.size(); idx++)
        {
            // prepare vector with scenario number and tenors
            std::vector<std::tuple<int, int>> scn_tenors;
            scn_tenors.push_back(std::tuple<int, int>(scn_no, this->info[cap_flr_idx].events[idx].date_end.get_date_int()));
            
            // get discounting factor
            std::vector<double> dfs = crvs.get_df(this->info[cap_flr_idx].crv_disc, scn_tenors);

            // store discount factor
            this->info[cap_flr_idx].events[idx].df = dfs[0];
        }

        // calculate NPV and accured interest in reference currency
        std::tuple<int, std::string> scn_ccy = std::tuple<int, std::string>(scn_no, ref_ccy_nm);
        double fx_rate = fx.get_fx(scn_ccy);

        this->info[cap_flr_idx].acc_int_ref_ccy = this->info[cap_flr_idx].acc_int * fx_rate;
        this->info[cap_flr_idx].npv_ref_ccy = this->info[cap_flr_idx].npv * fx_rate;

    }
}

// calculate NPV using multithreading
std::thread myCapsFloors::calc_npv_thrd(const int &scn_no, const myCurves &crvs, const myFx &fx, const std::string &ref_ccy_nm)
{
    std::thread worker(&myCapsFloors::calc_npv, this, scn_no, crvs, fx, ref_ccy_nm);
    return worker;
}

// write NPV into SQLite database file
void myCapsFloors::write_npv(mySQLite &db, const int &scn_no, const std::string &ent_nm, const std::string &ptf)
{
    // initiate dataframe structure
    dataFrame df;

    // add column names
    df.col_nms = {"scn_no", "ent_nm", "parent_id", "contract_id", "ptf", "acc_int", "npv", "acc_int_ref_ccy", "npv_ref_ccy"};

    // add column data type
    df.dtypes = {"INT", "CHAR", "CHAR", "CHAR", "CHAR", "FLOAT", "FLOAT", "FLOAT", "FLOAT"};
    
    // add values
    std::vector<std::vector<std::string>> values;
    std::vector<std::string> cap_flr;

    // go bond by bond
    for (int cap_flr_idx = 0; cap_flr_idx < this->info.size(); cap_flr_idx++)
    {
        cap_flr.clear();
        cap_flr.push_back(std::to_string(scn_no));
        cap_flr.push_back(this->info[cap_flr_idx].ent_nm);
        cap_flr.push_back(this->info[cap_flr_idx].parent_id);
        cap_flr.push_back(this->info[cap_flr_idx].contract_id);
        cap_flr.push_back(this->info[cap_flr_idx].ptf);
        cap_flr.push_back(std::to_string(this->info[cap_flr_idx].acc_int));
        cap_flr.push_back(std::to_string(this->info[cap_flr_idx].npv));
        cap_flr.push_back(std::to_string(this->info[cap_flr_idx].acc_int_ref_ccy));
        cap_flr.push_back(std::to_string(this->info[cap_flr_idx].npv_ref_ccy));
        values.push_back(cap_flr);
    }
    df.values = values;

    // create dataframe object
    myDataFrame tbl = myDataFrame(df);

    // delete old data based on scenario number, entity name and portfolio
    std::string sql = "DELETE FROM cap_flr_npv WHERE scn_no = " + std::to_string(scn_no) + " AND ent_nm = '" + ent_nm + "' AND ptf = '" + ptf + "';";
    db.exec(sql);

    // write dataframe into SQLite database file
    db.upload_tbl(tbl, "cap_flr_npv", false);
}
