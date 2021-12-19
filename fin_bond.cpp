#include <string>
#include <iostream>
#include <tuple>
#include <map>
#include <vector>
#include "lib_date.h"
#include "fin_curve.h"
#include "fin_fx.h"
#include "fin_date.h"
#include "fin_bond.h"

/*
 * AUXILIARY FUNCTIONS
 */

// return position index of event.date in std::vector events which is the closest to date
static int get_nearest_date_idx(const myDate &date, const std::vector<event> &events)
{
    // distances 
    int idx1_dist;
    int idx2_dist;

    // check the first event in events; we assume that the events are sorted chronologically
    if (date.get_days_no() <= events[0].date.get_days_no())
    {
        return 0;
    }

    // check the last event in events; we assume that the events are sorted chronologically
    if (date.get_days_no() >= events[events.size() - 1].date.get_days_no())
    {
        return events.size() - 1;
    }

    // go event by event in the std::vector; we assume that the events are sorted chronologically
    for (int idx = 0; idx < events.size() - 1; idx++)
    {
        // check the match on the current position index
        if (date.get_days_no() == events[idx].date.get_days_no())
        {
            return idx;
        }
        
        // check the match on the next position index
        if (date.get_days_no() == events[idx + 1].date.get_days_no())
        {
            return idx + 1;
        }

        // check the current interval defined by position indices idx and idx + 1
        idx1_dist = date.get_days_no() - events[idx].date.get_days_no();
        idx2_dist = date.get_days_no() - events[idx + 1].date.get_days_no();
        if ((idx1_dist > 0) & (idx2_dist < 0)) // date is within the interval
        {
            // return the nearest index
            if (idx1_dist < -idx2_dist)
            {
                return idx;
            }
            else
            {
                return idx + 1;
            }
        }
    }

    // this part of the code should not be hit (return should occur earlier)
    // but I have added it to prevent compiler from complaining about
    // missing return statement
    return 0;
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

            // portfolio
            bnd.ptf = rslt->tbl.values[bnd_idx][3];

            // account
            bnd.account = rslt->tbl.values[bnd_idx][4];

            // bond ISIN
            bnd.isin = rslt->tbl.values[bnd_idx][5];

            // comments
            bnd.comments = rslt->tbl.values[bnd_idx][6];

            // bond type, e.g. PAM, RGM, ZCB
            bnd.bnd_type = rslt->tbl.values[bnd_idx][7];

            // field indicating if the bond is fixed or floating
            bnd.is_fixed = rslt->tbl.values[bnd_idx][8].compare("1") == 0 ? true : false;

            // fixing type (forward rate vs. par-rate) for a floating bond
            aux = rslt->tbl.values[bnd_idx][9];
            if ((aux.compare("fwd") != 0) && (aux.compare("par") != 0) && (aux.compare("") != 0))
            {
                bnd.wrn_msg += "unsupported fixing type " + aux + ";";
                bnd.fix_type = "";
            }

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
                bnd.fix_type = "";
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
                bnd.fix_type = "";
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
                bnd.fix_type = "";
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
                bnd.fix_type = "";
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
                    bnd.fix_type = "";
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
                    bnd.fix_type = "";
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
                    bnd.fix_type = "";
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

            // reset NPV
            bnd.npv = 0.0;

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

        // generate std::vector of events

            // variables containing dates
            myDate date1;
            myDate date2;
            std::vector<myDate> * event_dates = new std::vector<myDate>;

            // events
            event evnt;
            std::vector<event> events;

            // position index
            int pos_idx;

            // date format, which is used to store date std::string in object myDate
            std::string date_format = "yyyymmdd";

            // find begining of the first active copoun period a create std::vector coupon events; we assume that all other events occur on coupon payment dates
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

                // create a std::vector of events based on coupon dates
                for (int idx = 0; idx < event_dates->size(); idx++)
                {
                    evnt.is_cpn_payment = true;
                    date1 = (*event_dates)[idx];
                    date2 = date1;
                    date1.remove(bnd.cpn_freq);
                    evnt.date = date2;
                    evnt.cpn_year_frac = day_count_method(date1, date2, bnd.dcm);

                    // indicated that coupon payment is fixed and therefore
                    // could be calculated without a scenario knowledge
                    if (bnd.is_fixed)
                    {
                        evnt.is_cpn_fix = true;
                    }
                    else
                    {
                        // even a floating bond has its first coupon payment
                        // fixed if it is not a forward contract
                        if (evnt.date.get_days_no() <= calc_date.get_days_no())
                        {
                            evnt.is_cpn_fix = true;
                        }
                        // coupon payments in fugure
                        else
                        {
                            evnt.is_cpn_fix = false;
                        }
                    }

                    events.emplace_back(evnt);
                }
            }
            else // zero coupon bond
            {
                evnt.date = bnd.maturity_date;
                evnt.is_cpn_payment = true;
                evnt.cpn_year_frac = 0.0;
                evnt.is_cpn_fix = true;
                events.emplace_back(evnt);
            }

            // add amortization information to events std::vector
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
                for (int idx = 0; idx < event_dates->size(); idx++)
                {
                    pos_idx = get_nearest_date_idx((*event_dates)[idx], events);
                    events[idx].is_amort = true;
                }

                // add nominal information; we assume that bnd.nominal contains the current bond nominal (i.e. taking potential previous amortizations
                // into account)
                for (int idx = 0; idx < events.size(); idx++)
                {
                    // add nominal
                    if (idx == 0) // first coupon date => use the current bond nominal
                    {
                        events[idx].nominal = bnd.nominal;
                    }
                    else // on the following coupon dates we use nominal from the previous coupon date
                    {
                        events[idx].nominal = events[idx - 1].nominal;
                    }

                    if (events[idx].is_amort)
                    {
                        events[idx].nominal -= bnd.amort;
                        events[idx].amort_payment = bnd.amort;
                    }
                    else
                    {
                        events[idx].amort_payment = 0.0;
                    }
                }
            }

            // add repricing information
            if (bnd.cpn_freq.compare("") != 0)
            {
                // iterate until you find the first repricing date after calculation date
                date1 = bnd.first_fix_date;
                while (date1.get_days_no() < calc_date.get_days_no())
                {
                    date1.add(bnd.fix_freq);
                }

                // date1 is the first fixing date => we generate repricing dates till maturity
                event_dates = create_date_serie(date1.get_date_str(), bnd.maturity_date.get_date_str(), bnd.fix_freq, date_format);

                // go repricing date by repricing date and assign it the nearest coupon date; we assume that amortizations happen on coupon dates
                for (int idx = 0; idx < event_dates->size(); idx++)
                {
                    pos_idx = get_nearest_date_idx((*event_dates)[idx], events);
                    events[idx].is_repricing = true;
                }

                // repricing dates
                if (bnd.fix_type.compare("") != 0)
                {
                    for (int idx = 0; idx < events.size(); idx++)
                    {
                        // get boundary dates
                        if (events[idx].is_repricing)
                        {
                            date1 = events[idx].date;
                            date2 = date1;
                            date2.add(bnd.fix_freq);
                            pos_idx = get_nearest_date_idx(date2, events);
                            date2 = events[pos_idx].date; 
                       }
                       
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
                            for (int idx2; idx2 < events.size(); idx2++)
                            {
                                // check that coupon date falls within
                                // the boundary repricing dates
                                if ((events[idx2].date.get_days_no() >= date1.get_days_no()) &&
                                    (events[idx2].date.get_days_no() <= date2.get_days_no()))
                                {
                                    events[idx].repricing_dates.emplace_back(events[idx2].date);
                                }
                            }
                       }
                    }
                }
            }

            // calculate fixed cupoun payments and accrued interest
            for (int idx = 0; idx < events.size(); idx++)
            {
                if (events[idx].is_cpn_fix)
                {
                    // calculate accrued interest for the first coupon payment
                    // if neccessary
                    if (!bnd.is_acc_int && (idx == 0))
                    {
                        if (events[idx].date.get_days_no() <= calc_date.get_days_no())
                        {
                            // year fraction for the first coupon payment
                            float acc_int_year_frac =
                                day_count_method(events[idx].date, calc_date, bnd.dcm);
                            bnd.acc_int =
                                events[idx].cpn * events[idx].nominal * acc_int_year_frac;
                        }
                        // in case of a forward contract accrued interest is zero
                        else
                        {
                            bnd.acc_int = 0.0;
                        }

                    // calculate fixed coupon payment
                    events[idx].is_repricing = false;
                    events[idx].is_cpn_fix = true;
                    events[idx].cpn = bnd.cpn_rate;
                    events[idx].cpn_payment =
                        events[idx].cpn * events[idx].cpn_year_frac * events[idx].nominal;

                    // check that the accured interest is not greater than the
                    // first coupon payment; please note we have to take into
                    // account also possibility of negative nominal
                    if (idx == 0)
                    {
                        // positive coupon rate
                        if (events[idx].cpn > 0.0)
                        {
                            // positive nominal
                            if (events[idx].nominal > 0.0)
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
                            if (events[idx].nominal > 0.0)
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

            // delete unused pointers
            delete event_dates;
        }
    }
}

/*
 * OBJECT FUNCTIONS
 */


