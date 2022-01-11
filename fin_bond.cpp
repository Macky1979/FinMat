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
#include "fin_bond.h"
#include "lib_aux.h"

/*
 * AUXILIARY FUNCTIONS
 */

// extract vector of dates from vector of events
static std::vector<myDate> extract_dates_from_events(const std::vector<bnd_event> &events, const std::string &type)
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

myBonds::myBonds(const mySQLite &db, const std::string &sql, const myDate &calc_date)
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
    for (int bnd_idx = 0; bnd_idx < rslt->tbl.values.size(); bnd_idx++)
    {
        // load bond information and perform some sanity checks
        bnd_info bnd;

            // entity name
            bnd.ent_nm = rslt->tbl.values[bnd_idx][0];

            // parent id; useful to bind two or more bonds together to create a new product like IRS
            bnd.parent_id = rslt->tbl.values[bnd_idx][1];

            // contract id
            bnd.contract_id = rslt->tbl.values[bnd_idx][2];

            // issuer id
            bnd.issuer_id = rslt->tbl.values[bnd_idx][3];

            // portfolio
            bnd.ptf = rslt->tbl.values[bnd_idx][4];

            // account
            bnd.account = rslt->tbl.values[bnd_idx][5];

            // bond ISIN
            bnd.isin = rslt->tbl.values[bnd_idx][6];

            // comments
            bnd.comments = rslt->tbl.values[bnd_idx][7];

            // bond type, e.g. PAM, RGM, ZCB
            bnd.bnd_type = rslt->tbl.values[bnd_idx][8];

            // fixing type - "fwd" and "par" for a floating bond vs.
            // "fix" for a fixed bond
            aux = rslt->tbl.values[bnd_idx][9];
            if ((aux.compare("fwd") != 0) && (aux.compare("par") != 0) && (aux.compare("fix") != 0))
            {
                bnd.wrn_msg += "unsupported fixing type " + aux + ";";
                bnd.fix_type = "fix";
            }

            if (aux.compare("fix") == 0)
            {
                bnd.is_fixed = true;
            }
            else
            {
                bnd.is_fixed = false;
            }
            bnd.fix_type = aux;

            // bond rating
            bnd.rtg = rslt->tbl.values[bnd_idx][10];

            // bond currency, e.g. EUR, CZK
            bnd.ccy_nm = rslt->tbl.values[bnd_idx][11];

            // bond nominal
            bnd.nominal = stod(rslt->tbl.values[bnd_idx][12]);

            // bond deal date, i.e. date when the bond entered / will enter into books
            bnd.deal_date = myDate(stoi(rslt->tbl.values[bnd_idx][13]));

            // bond maturity date
            bnd.maturity_date = myDate(stoi(rslt->tbl.values[bnd_idx][14]));

            // day count method used to calculate coupon payment
            bnd.dcm = rslt->tbl.values[bnd_idx][15];

            // bond accrued interest
            aux = rslt->tbl.values[bnd_idx][16];
            if (aux.compare("") == 0) // accrued interest not provided
            {
                bnd.wrn_msg += "accrued interest not provided;";
                bnd.is_acc_int = false;
                bnd.acc_int = 0.0;
            }
            else // accrued interest provided
            {
                bnd.is_acc_int = true;
                bnd.acc_int = stod(aux);
            }

            // coupon rate
            aux = rslt->tbl.values[bnd_idx][17];
            if (aux.compare("") == 0) // coupon rate not provided
            {
                bnd.wrn_msg += "coupon rate not provided;";
                bnd.is_fixed = true;
                bnd.is_acc_int = true;
                bnd.acc_int = 0.0;
                bnd.cpn_rate = 0.0;
                bnd.cpn_freq = "";
                bnd.fix_type = "fix";
                bnd.first_cpn_date = bnd.maturity_date;
            }
            else // coupon rate provided
            {
                bnd.cpn_rate = stod(aux);
            }
             
            // first coupon date
            aux = rslt->tbl.values[bnd_idx][18];
            if (aux.compare("") == 0) // first coupon date not provided
            {
                bnd.wrn_msg += "first coupon date not provided;";
                bnd.is_fixed = true;
                bnd.is_acc_int = true;
                bnd.acc_int = 0.0;
                bnd.cpn_rate = 0.0;
                bnd.cpn_freq = "";
                bnd.fix_type = "fix";
                bnd.first_cpn_date = bnd.maturity_date;
            }
            else // the first coupon date provided
            {
                bnd.first_cpn_date = myDate(stoi(aux));
            }

            // coupon frequency
            aux = rslt->tbl.values[bnd_idx][19];
            if ((aux.compare("") == 0) && bnd.cpn_rate != 0.0) // coupon frequency not provided for non-zero coupon rate
            {
                bnd.wrn_msg += "coupon frequency not provided for coupon bearing bond;";
                bnd.is_fixed = true;
                bnd.is_acc_int = true;
                bnd.acc_int = 0.0;
                bnd.cpn_rate = 0.0;
                bnd.cpn_freq = "";
                bnd.fix_type = "fix";
                bnd.first_cpn_date = bnd.maturity_date;
            }
            else if ((aux.compare("") != 0) && (bnd.cpn_rate == 0)) // copoun frequency provided for zero coupon rate
            {
                bnd.wrn_msg += "coupon frequency provided for zero coupon bond;";
                bnd.is_fixed = true;
                bnd.is_acc_int = true;
                bnd.acc_int = 0.0;
                bnd.cpn_rate = 0.0;
                bnd.cpn_freq = "";
                bnd.fix_type = "fix";
                bnd.first_cpn_date = bnd.maturity_date;
            }
            else // coupon frequency provided for non-zero coupon rate
            {
                bnd.cpn_freq = aux;
            }

            // first fixing date of a floating bond
            aux = rslt->tbl.values[bnd_idx][20];
            if (aux.compare("") == 0) // fixing date not provided
            {
                if (!bnd.is_fixed) // floating bond
                {
                    bnd.wrn_msg += "first fixing date not provided for a floating bond;";
                    bnd.is_fixed = true;
                    bnd.fix_freq = "";
                    bnd.fix_type = "fix";
                }
            }
            else // fixing date provided
            {
                if (!bnd.is_fixed) // floating bond
                {
                    bnd.first_fix_date = myDate(stoi(aux));
                }
                else // fixed bond
                {
                    bnd.wrn_msg += "first fixing date provided for a fixed bond;";
                }
            }
            
            // fixing frequency
            aux = rslt->tbl.values[bnd_idx][21];
            if (aux.compare("") == 0) // fixing frequency not provided
            {
                if (!bnd.is_fixed) // floating bond
                {
                    bnd.wrn_msg += "fixing frequency not provided a floating bond";
                    bnd.is_fixed = true;
                    bnd.fix_freq = "";
                    bnd.fix_type = "fix";
                }
                else // fixed bond
                {
                    bnd.fix_freq = "";
                }
            }
            else // fixing frequency provided
            {
                if (!bnd.is_fixed) // floating bond
                {
                    bnd.fix_freq = aux;
                }
                else // fixed bond
                {
                    bnd.wrn_msg += "fixing frequency provided for a fixed bond;";
                    bnd.fix_freq = "";
                }
            }

            // amortization
            std::string first_amort_date = rslt->tbl.values[bnd_idx][22];
            std::string amort_freq = rslt->tbl.values[bnd_idx][23];
            std::string amort = rslt->tbl.values[bnd_idx][24];
            
            if (amort.compare("") == 0) // amortization amount not provided
            {
                bnd.amort = 0.0;

                if (amort_freq.compare("") != 0) // amortization frequency provided
                {
                    bnd.wrn_msg += "amortization frequency provided for a bond with zero amortization amount;";
                }
                bnd.amort_freq = "";

                if (first_amort_date.compare("") != 0) // amortization frequency provided 
                {
                    bnd.wrn_msg += "first amortization date provided for a bond with zero amortization amount;";
                }
            }
            else // amortization amount provided
            {
                bnd.amort = stod(amort);

                if (amort_freq.compare("") == 0) // amortization frequency not provided
                {
                    bnd.wrn_msg += "amortization frequency not provided for a bond with non-zero amortization amount;";
                    bnd.amort = 0.0;
                    bnd.amort_freq = "";
                }
                else // amortization frequency provided
                {
                    bnd.amort_freq = amort_freq;
                }

                if (first_amort_date.compare("") == 0) // first amortization date not provided
                {
                    bnd.wrn_msg += "first amortization date not provided for a bond with non-zero amortization amount;";
                    bnd.amort = 0.0;
                    bnd.amort_freq = "";
                }
                else // first amortization date provided
                {
                    bnd.first_amort_date = myDate(stoi(first_amort_date));
                }
            }
            
            // multiplier applied on bond repricing rate
            aux = rslt->tbl.values[bnd_idx][25];
            if (aux.compare("") == 0) // rate multiplier not provided
            {
                if (!bnd.is_fixed) // floating bond
                {
                    bnd.wrn_msg += "rate multiplier not provided for a floating bond;";
                    bnd.rate_mult = 1.0;
                }
                else // fixed bond
                {
                    bnd.rate_mult = 0.0;
                }
                
            }
            else // rate multiplied provided
            {
                if (!bnd.is_fixed) // floating bond
                {
                    bnd.rate_mult = stod(aux);
                }
                else // fixed bond
                {
                    bnd.wrn_msg += "rate multiplier provided for a fixed bond;";
                    bnd.rate_mult = 0.0;
                }
            }

            // spread to be added to a repricing rate
            aux = rslt->tbl.values[bnd_idx][26];
            if (aux.compare("") == 0) // spread not specified
            {
                if (!bnd.is_fixed) // floating bond
                {
                    bnd.wrn_msg += "repricing spread not provided for a flating bond;";
                }
                bnd.rate_add = 0.0;
            }
            else // spread specified
            {
                if (!bnd.is_fixed) // floating bond
                {
                    bnd.rate_add = stod(aux);
                }
                else // fixed bond
                {
                    bnd.wrn_msg += "repricing spread provided for a fixed bond;";
                    bnd.rate_add = 0.0;
                }
            }
            
            // discounting curve
            bnd.crv_disc = rslt->tbl.values[bnd_idx][27];

            // repricing curve
            aux = rslt->tbl.values[bnd_idx][28];
            if (aux.compare("") == 0) // repricing curve not specified
            {
                if (!bnd.is_fixed) // floating bond
                {
                    bnd.wrn_msg += "repricing curve not provided for a floating bond;";
                    bnd.is_fixed = true;
                    bnd.fix_freq = "";
                    bnd.fix_type = "fix";
                }
                bnd.crv_fwd = "";
            }
            else // repricing curve specified
            {
                if (!bnd.is_fixed) // floating bond
                {
                    bnd.crv_fwd = aux;
                }
                else // fixed bond
                {
                    bnd.wrn_msg += "repricing curve specified for a fixed bond;";
                    bnd.crv_fwd = "";
                }
            }

        // perform other sanity checks
       
            // deal date
            if (bnd.deal_date.get_days_no() > bnd.maturity_date.get_days_no())
            {
                bnd.wrn_msg += "deal date cannot by greater than maturity date;";
                bnd.deal_date = bnd.maturity_date;
            }

            // first coupon date
            if (bnd.first_cpn_date.get_days_no() <= bnd.deal_date.get_days_no())
            {
                bnd.wrn_msg += "first coupon date cannot be lower or equal to deal date;";
                bnd.first_cpn_date = bnd.deal_date;
                bnd.first_cpn_date.add(bnd.cpn_freq);
                while (bnd.first_cpn_date.get_days_no() < bnd.deal_date.get_days_no())
                {
                    bnd.first_cpn_date.add(bnd.cpn_freq);
                }
            }

            if (bnd.first_cpn_date.get_days_no() > bnd.maturity_date.get_days_no())
            {
                bnd.wrn_msg += "first coupon date cannot be greater than maturity date;";
                bnd.first_cpn_date = bnd.maturity_date;
            }

            // first fixing date
            if (!bnd.is_fixed)
            {
                if (bnd.first_fix_date.get_days_no() <= bnd.deal_date.get_days_no())
                {
                    bnd.wrn_msg += "first fixing date cannot be lower or equal to deal date;";
                    bnd.first_fix_date = bnd.first_cpn_date;
                }

                if (bnd.first_fix_date.get_days_no() > bnd.maturity_date.get_days_no())
                {
                    bnd.wrn_msg += "first fixing date cannot be greater than maturity date;";
                    bnd.first_fix_date = bnd.first_cpn_date;
                }

                if (bnd.first_fix_date.get_days_no() <= bnd.first_cpn_date.get_days_no())
                {
                    bnd.wrn_msg += "first fixing date cannot be lower or equal to the first copoun date;";
                    bnd.first_fix_date = bnd.first_cpn_date;
                }
            }

            // first amortization date
            if (bnd.amort != 0)
            {
                if (bnd.first_amort_date.get_days_no() <= bnd.deal_date.get_days_no())
                {
                    bnd.wrn_msg += "first amortization date cannot be lower or equal to deal date;";
                    bnd.first_amort_date = bnd.first_cpn_date;
                }

                if (bnd.first_amort_date.get_days_no() > bnd.maturity_date.get_days_no())
                {
                    bnd.wrn_msg += "first amortization date cannot be greater than maturity date;";
                    bnd.first_amort_date = bnd.first_cpn_date;
                }

                if (bnd.first_amort_date.get_days_no() <= bnd.first_cpn_date.get_days_no())
                {
                    bnd.wrn_msg += "first amortization date cannot be lower or equal to the first copoun date;";
                    bnd.first_amort_date = bnd.first_cpn_date;
                }
            }

        // check frequencies
        
            // amortization frequency
            if (bnd.amort_freq.compare("") != 0)
            {
                if (eval_freq(bnd.amort_freq) < eval_freq(bnd.cpn_freq))
                {
                    bnd.wrn_msg += "amortization frequency cannot be lower than coupon frequency;";
                    bnd.amort_freq = bnd.cpn_freq;
                }
            }

            // repricing frequency
            if (bnd.fix_freq.compare("") != 0)
            {
                if (eval_freq(bnd.fix_freq) < eval_freq(bnd.cpn_freq))
                {
                    bnd.wrn_msg += "repricing frequency cannot be lower than coupon frequency;";
                    bnd.fix_freq = bnd.cpn_freq;
                }
            }

        // generate vector of events

            // variables containing dates
            myDate date1;
            myDate date2;

            // events
            bnd_event evnt;
            std::vector<bnd_event> events;

            // position index
            int pos_idx;

            // date format, which is used to store date string in object myDate
            std::string date_format = "yyyymmdd";

            // find date of the first copoun payment and a create vector coupon events;
            // we assume that all other events occur on coupon payment dates
            if (bnd.cpn_freq.compare("") != 0) // coupon bond
            {
                // iterate until you find the first coupon period that does not preceed calculation date
                date1 = bnd.first_cpn_date;
                date2 = bnd.first_cpn_date;
                date2.remove(bnd.cpn_freq);
                while ((date1.get_days_no() < calc_date.get_days_no()) & (date2.get_days_no() < calc_date.get_days_no()))
                {
                    date1 = date2;
                    date2.add(bnd.cpn_freq);
                }
                
                // date1 represents the beginning date of such coupon period => we generate coupon dates till maturity
                event_dates = create_date_serie(date1.get_date_str(), bnd.maturity_date.get_date_str(), bnd.cpn_freq, date_format);     

                // create a vector of events based on coupon dates
                for (int idx = 0; idx < event_dates.size(); idx++)
                {
                    date1 = event_dates[idx];
                    date2 = date1;
                    date1.remove(bnd.cpn_freq);
                    evnt.date_begin = date1; // beging of coupon period
                    evnt.date_end = date2; // end of coupon period => date of coupon payment
                    evnt.cpn_year_frac = day_count_method(date1, date2, bnd.dcm);
                    evnt.is_cpn_payment = true;

                    // indicated that coupon payment is fixed and therefore
                    // could be calculated without a scenario knowledge
                    if (bnd.is_fixed)
                    {
                        evnt.is_cpn_fixed = true;
                    }
                    else
                    {
                        // even a floating bond has its first coupon payment
                        // fixed if it is not a forward contract
                        if (evnt.date_begin.get_days_no() < bnd.first_fix_date.get_days_no())
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
                evnt.date_begin = bnd.maturity_date;
                evnt.date_end = bnd.maturity_date;
                evnt.cpn_year_frac = 0.0;
                evnt.is_cpn_payment = true;
                evnt.is_cpn_fixed = true;
                events.emplace_back(evnt);
            }

            // extract begin and end dates of the coupon periods
            begin_cpn_dates = extract_dates_from_events(events, "date_begin");
            end_cpn_dates = extract_dates_from_events(events, "date_end");

            // add amortization information to events vector
            if (bnd.amort_freq.compare("") != 0)
            {
                // iterate until you find the first amortization date after calculation date
                date1 = bnd.first_amort_date;
                while (date1.get_days_no() < calc_date.get_days_no())
                {
                    date1.add(bnd.amort_freq);
                }

                // date1 is the first amortization date => we generate amortizaton dates till maturity
                event_dates = create_date_serie(date1.get_date_str(), bnd.maturity_date.get_date_str(), bnd.amort_freq, date_format);

                // go amortization date by amortization date and assign it the nearest coupon date; we assume that amortizations happen on coupon dates
                for (int idx = 0; idx < event_dates.size(); idx++)
                {
                    // we assign amortization flag based on begining coupon date
                    if (event_dates[idx].get_date_int() < bnd.maturity_date.get_date_int())
                    {
                        pos_idx = get_nearest_idx(event_dates[idx], end_cpn_dates, "abs");
                        events[pos_idx].amort_flg = true;
                    }
                }
            }

            // add nominal information; we assume that bnd.nominal contains the current bond nominal (i.e. taking potential previous amortizations
            // into account)
            for (int idx = 0; idx < events.size(); idx++)
            {
                // set amortization payment to zero by default
                events[idx].amort_payment = 0.0;

                // set nominal
                if (idx == 0) // first coupon date => use the current bond nominal
                {
                    events[idx].nominal_begin = bnd.nominal;
                    events[idx].nominal_end = bnd.nominal;
                }
                else // on the following coupon dates we use nominal from the previous coupon date
                {
                    events[idx].nominal_begin = events[idx - 1].nominal_end;
                    events[idx].nominal_end = events[idx - 1].nominal_end;
                }

                // adjust for amortization payment
                if (events[idx].amort_flg)
                {
                    events[idx].nominal_end -= bnd.amort;
                    events[idx].amort_payment = bnd.amort;
                }

                // the last coupon payment is accompanied by full nominal repayment
                if (idx == events.size() - 1)
                {
                    events[idx].amort_payment = events[idx].nominal_begin;
                    events[idx].nominal_end = 0.0;
                }
            }

            // add repricing information
            if (bnd.fix_freq.compare("") != 0)
            {
                // iterate until you find the first repricing date after calculation date
                date1 = bnd.first_fix_date;
                while (date1.get_days_no() < calc_date.get_days_no())
                {
                    date1.add(bnd.fix_freq);
                }

                // date1 is the first repricing date => we generate repricing dates till maturity
                event_dates = create_date_serie(date1.get_date_str(), bnd.maturity_date.get_date_str(), bnd.fix_freq, date_format);

                // determine in which coupon periods there will be repricing
                for (int idx = 0; idx < event_dates.size(); idx++)
                {
                    // we assign repricing flag based on begining coupon date
                    if (event_dates[idx].get_date_int() < bnd.maturity_date.get_date_int())
                    {
                        pos_idx = get_nearest_idx(event_dates[idx], begin_cpn_dates, "abs");
                        events[pos_idx].fix_flg = true;
                    }
                }

                // repricing dates
                if (bnd.fix_type.compare("fix") != 0)
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
                            date2.add(bnd.fix_freq);
                       
                            // forward rate repricing - only the boundary dates
                            if (bnd.fix_type.compare("fwd") == 0)
                            {
                                events[idx].repricing_dates.emplace_back(date1);
                                events[idx].repricing_dates.emplace_back(date2);
                            }

                            // par rate repricing - not only boundary repricing
                            // dates but also in between coupon dates
                            else if (bnd.fix_type.compare("par") == 0)
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
                    if (!bnd.is_acc_int && (idx == 0))
                    {
                        if (events[idx].date_begin.get_days_no() <= calc_date.get_days_no())
                        {
                            // year fraction for the first coupon payment
                            double acc_int_year_frac = day_count_method(events[idx].date_begin, calc_date, bnd.dcm);
                            bnd.acc_int = bnd.cpn_rate * events[idx].nominal_begin * acc_int_year_frac;
                        }
                        // in case of a forward contract accrued interest is zero
                        else
                        {
                            bnd.acc_int = 0.0;
                        }
                    }

                    // calculate fixed coupon payment
                    events[idx].cpn = bnd.cpn_rate;
                    events[idx].cpn_payment = events[idx].cpn * events[idx].cpn_year_frac * events[idx].nominal_begin;

                    // check that the accured interest is not greater than the
                    // first coupon payment; please note we have to take into
                    // account also possibility of negative nominal
                    if (idx == 0)
                    {
                        // positive coupon rate
                        if (events[idx].cpn > 0.0)
                        {
                            // positive nominal
                            if (events[idx].nominal_begin > 0.0)
                            {
                                // accrued interest is greater than the
                                // first coupon payment
                                if (bnd.acc_int > events[idx].cpn_payment)
                                {
                                    bnd.acc_int = events[idx].cpn_payment;
                                }
                            }
                            // negative nominal
                            else
                            {
                                // accrued interest is greater than the
                                // first coupon payment
                                if (bnd.acc_int < events[idx].cpn_payment)
                                {
                                    bnd.acc_int = events[idx].cpn_payment;
                                }
                            }
                        }
                        // negative coupon rate
                        else
                        {
                            // positive nominal
                            if (events[idx].nominal_begin > 0.0)
                            {
                                // accrued interest is greater than the
                                // first coupon payment
                                if (bnd.acc_int < events[idx].cpn_payment)
                                {
                                    bnd.acc_int = events[idx].cpn_payment;
                                }
                            }
                            // negative nominal
                            else
                            {
                                // accrued interest is greater than the
                                // first coupon payment
                                if (bnd.acc_int > events[idx].cpn_payment)
                                {
                                    bnd.acc_int = events[idx].cpn_payment;
                                }
                            }
                        }
                    }

                    // total cash-flow generated by the bond
                    events[idx].cf = events[idx].amort_payment + events[idx].cpn_payment;
                }
            }
        
            // add events into bond structure
            bnd.events = events;

            // add bond to vector of bonds
            this->info.emplace_back(bnd);
    
        }

    // delete unused pointers
    delete rslt;
}

/*
 * OBJECT FUNCTIONS
 */

// split the object
std::vector<myBonds> myBonds::split(const int &threads_no)
{

    // determine position indicies of the newly created vectors
    std::vector<coordinates<int>> indicies = split_vector(this->info.size(), threads_no);

    // split original object myAnnuities into several smaller objects
    std::vector<myBonds> bnds;
    for (int thread_idx = 0; thread_idx < threads_no; thread_idx++)
    {
        // determine the first and the last position index
        std::vector<bnd_info>::const_iterator first = this->info.begin() + indicies[thread_idx].x;
        std::vector<bnd_info>::const_iterator last = this->info.begin() + indicies[thread_idx].y + 1;

        // extract bond information
        std::vector<bnd_info> info(first, last);

        // create new smaller object based on the position indicies
        myBonds bnd(info);

        // add the new object to the vector
        bnds.emplace_back(bnd);
    }

    // release memory by clearing the original object
    this->clear();

    // return vector of objects
    return bnds;
}

// merge several objects
void myBonds::merge(std::vector<myBonds> &bnds)
{
    // pre-allocate memory
    int final_vector_size = 0;
    std::vector<bnd_info> info;
    
    for (int idx = 0; idx < bnds.size(); idx++)
    {
        final_vector_size += bnds[idx].info.size();
    }

    info.reserve(final_vector_size);

    // merge individual vectors
    for (int idx = 0; idx < bnds.size(); idx++)
    {
        info.insert(info.end(), bnds[idx].info.begin(), bnds[idx].info.end());
        bnds[idx].clear(); // release memory by clearing individual objects
    }

    // copy merged vectors into the current object
    this->info = info;
}

// calculate NPV
void myBonds::calc_npv(const int &scn_no, const myCurves &crvs, const myFx &fx, const std::string &ref_ccy_nm)
{
    // variables 
    std::vector<std::tuple<int, int>> scn_tenors;

    // go bond by bond
    for (int bnd_idx = 0; bnd_idx < this->info.size(); bnd_idx++)
    {
        // calculate repricing rates for floating bonds
        if (!this->info[bnd_idx].is_fixed)
        {
            // go event by event
            for (int idx = 0; idx < this->info[bnd_idx].events.size(); idx++)
            {
                // coupon is already fixed
                if (this->info[bnd_idx].events[idx].is_cpn_fixed | !this->info[bnd_idx].events[idx].fix_flg)
                {
                    if (idx > 0)
                    {
                        this->info[bnd_idx].events[idx].cpn = this->info[bnd_idx].events[idx - 1].cpn;
                    }
                }
                // calculate forward rate / par rate for unfixed coupon rates
                else
                {
                    // prepare vector with scenario number and tenors
                    scn_tenors.clear();
                    for (int idx2 = 0; idx2 < this->info[bnd_idx].events[idx].repricing_dates.size(); idx2++)
                    {
                        scn_tenors.push_back(std::tuple<int, int>(scn_no, this->info[bnd_idx].events[idx].repricing_dates[idx2].get_date_int()));
                    }

                    // apply forward rate
                    if (this->info[bnd_idx].fix_type.compare("fwd") == 0)
                    {
                        // get forward rate
                        std::vector<double> fwds = crvs.get_fwd_rate(this->info[bnd_idx].crv_fwd, scn_tenors, this->info[bnd_idx].dcm);

                        // use the forward rate as a coupon rate
                        this->info[bnd_idx].events[idx].cpn = fwds[0];
                    }
                    // apply par-rate
                    else
                    {
                        // get par-rate
                        int par_step = this->info[bnd_idx].events[idx].repricing_dates.size() - 1;
                        std::vector<double> pars = crvs.get_par_rate(this->info[bnd_idx].crv_fwd, scn_tenors, this->info[bnd_idx].events[idx].par_nominals_begin, this->info[bnd_idx].events[idx].par_nominals_end, par_step, this->info[bnd_idx].dcm);

                        // user the par-rate as a coupon rate
                        this->info[bnd_idx].events[idx].cpn = pars[0];
                    }

                    // apply multiplier and spread
                    this->info[bnd_idx].events[idx].cpn = this->info[bnd_idx].events[idx].cpn * this->info[bnd_idx].rate_mult + this->info[bnd_idx].rate_add;
                }
            }

            // calculate coupon payment and cash-flow
            for (int idx = 0; idx < this->info[bnd_idx].events.size(); idx++)
            {
                this->info[bnd_idx].events[idx].cpn_payment = this->info[bnd_idx].events[idx].nominal_begin * this->info[bnd_idx].events[idx].cpn * this->info[bnd_idx].events[idx].cpn_year_frac;
                this->info[bnd_idx].events[idx].cf = this->info[bnd_idx].events[idx].cpn_payment + this->info[bnd_idx].events[idx].amort_payment;
            }
        }

        // calculate discount factors and total bond NPV
        for (int idx = 0; idx < this->info[bnd_idx].events.size(); idx++)
        {
            // prepare vector with scenario number and tenors
            std::vector<std::tuple<int, int>> scn_tenors;
            scn_tenors.push_back(std::tuple<int, int>(scn_no, this->info[bnd_idx].events[idx].date_end.get_date_int()));
            
            // get discounting factor
            std::vector<double> dfs = crvs.get_df(this->info[bnd_idx].crv_disc, scn_tenors);

            // store discount factor
            this->info[bnd_idx].events[idx].df = dfs[0];

            // update NPV
            this->info[bnd_idx].npv += this->info[bnd_idx].events[idx].cf * this->info[bnd_idx].events[idx].df;

        }

        // calculate NPV and accured interest in reference currency
        std::tuple<int, std::string> scn_ccy = std::tuple<int, std::string>(scn_no, ref_ccy_nm);
        double fx_rate = fx.get_fx(scn_ccy);

        this->info[bnd_idx].acc_int_ref_ccy = this->info[bnd_idx].acc_int * fx_rate;
        this->info[bnd_idx].npv_ref_ccy = this->info[bnd_idx].npv * fx_rate;

    }
}

// calculate NPV using multithreading
std::thread myBonds::calc_npv_thrd(const int &scn_no, const myCurves &crvs, const myFx &fx, const std::string &ref_ccy_nm)
{
    std::thread worker(&myBonds::calc_npv, this, scn_no, crvs, fx, ref_ccy_nm);
    return worker;
}

// write NPV into SQLite database file
void myBonds::write_npv(mySQLite &db, const int &scn_no, const std::string &ent_nm, const std::string &ptf)
{
    // initiate dataframe structure
    dataFrame df;

    // add column names
    df.col_nms = {"scn_no", "ent_nm", "parent_id", "contract_id", "ptf", "acc_int", "npv", "acc_int_ref_ccy", "npv_ref_ccy"};

    // add column data type
    df.dtypes = {"INT", "CHAR", "CHAR", "CHAR", "CHAR", "FLOAT", "FLOAT", "FLOAT", "FLOAT"};
    
    // add values
    std::vector<std::vector<std::string>> values;
    std::vector<std::string> bnd;

    // go bond by bond
    for (int bnd_idx = 0; bnd_idx < this->info.size(); bnd_idx++)
    {
        bnd.clear();
        bnd.push_back(std::to_string(scn_no));
        bnd.push_back(this->info[bnd_idx].ent_nm);
        bnd.push_back(this->info[bnd_idx].parent_id);
        bnd.push_back(this->info[bnd_idx].contract_id);
        bnd.push_back(this->info[bnd_idx].ptf);
        bnd.push_back(std::to_string(this->info[bnd_idx].acc_int));
        bnd.push_back(std::to_string(this->info[bnd_idx].npv));
        bnd.push_back(std::to_string(this->info[bnd_idx].acc_int_ref_ccy));
        bnd.push_back(std::to_string(this->info[bnd_idx].npv_ref_ccy));
        values.push_back(bnd);
    }
    df.values = values;

    // create dataframe object
    myDataFrame tbl = myDataFrame(df);

    // delete old data based on scenario number, entity name and portfolio
    std::string sql = "DELETE FROM bnd_npv WHERE scn_no = " + std::to_string(scn_no) + " AND ent_nm = '" + ent_nm + "' AND ptf = '" + ptf + "';";
    db.exec(sql);

    // write dataframe into SQLite database file
    db.upload_tbl(tbl, "bnd_npv", false);
}
