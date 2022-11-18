#include <string>
#include <iostream>
#include <tuple>
#include <map>
#include <vector>
#include <memory>
#include <cmath>
#include "lib_date.h"
#include "lib_dataframe.h"
#include "fin_curve.h"
#include "fin_fx.h"
#include "fin_date.h"
#include "lib_aux.h"
#include "lib_math.h"
#include "fin_cap_floor.h"

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
    std::vector<myDate> begin_int_dates;
    std::vector<myDate> end_int_dates;

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

            // cap / floor ISIN
            cap_flr.isin = rslt->tbl.values[cap_flr_idx][6];

            // rating
            cap_flr.rtg = rslt->tbl.values[cap_flr_idx][7];

            // comments
            cap_flr.comments = rslt->tbl.values[cap_flr_idx][8];

            // cap / floor / collar
            cap_flr.cap_floor_type = rslt->tbl.values[cap_flr_idx][9];

            // fixing type - "fwd" and "par"
            aux = rslt->tbl.values[cap_flr_idx][10];
            if ((aux.compare("fwd") != 0) && (aux.compare("par") != 0))
            {
                cap_flr.wrn_msg += "unsupported fixing type " + aux + ";";
                cap_flr.fix_type = "fwd";
            }
            else
            {
                cap_flr.fix_type = aux;
            }

            // currency, e.g. EUR, CZK
            cap_flr.ccy_nm = rslt->tbl.values[cap_flr_idx][11];

            // nominal
            cap_flr.nominal = stod(rslt->tbl.values[cap_flr_idx][12]);

            // calculation date
            cap_flr.calc_date = calc_date;

            // value date
            cap_flr.value_date = myDate(stoi(rslt->tbl.values[cap_flr_idx][13]));

            // maturity date
            cap_flr.maturity_date = myDate(stoi(rslt->tbl.values[cap_flr_idx][14]));

            // day count method used to calculate interent payment
            cap_flr.dcm = rslt->tbl.values[cap_flr_idx][15];
            
            // cap rate
            aux = rslt->tbl.values[cap_flr_idx][16];
            if (aux.compare("") == 0)
            {
                cap_flr.cap_rate = 100;
            }
            else
            {
                cap_flr.cap_rate = stod(aux);
            }
            
            // cap volatility surface
            cap_flr.cap_vol_surf = rslt->tbl.values[cap_flr_idx][17];

            // floor rate
            aux = rslt->tbl.values[cap_flr_idx][18];
            if (aux.compare("") == 0)
            {
                cap_flr.floor_rate = -100;
            }
            else
            {
                cap_flr.floor_rate = stod(aux);
            }
            
            // floor volatility surface
            cap_flr.floor_vol_surf = rslt->tbl.values[cap_flr_idx][19];

            // first interest payment date
            cap_flr.first_int_date = myDate(stoi(rslt->tbl.values[cap_flr_idx][20]));

            // interest payment frequency
            cap_flr.int_freq = rslt->tbl.values[cap_flr_idx][21];

            // first fixing date
            cap_flr.first_fix_date = myDate(stoi(rslt->tbl.values[cap_flr_idx][22]));
            
            // fixing frequency
            cap_flr.fix_freq = rslt->tbl.values[cap_flr_idx][23];

            // amortization
            std::string first_amort_date = rslt->tbl.values[cap_flr_idx][24];
            std::string amort_freq = rslt->tbl.values[cap_flr_idx][25];
            std::string amort = rslt->tbl.values[cap_flr_idx][26];
            
            if (amort.compare("") == 0) // amortization amount not provided
            {
                cap_flr.amort = 0.0;

                if (amort_freq.compare("") != 0) // amortization frequency provided
                {
                    cap_flr.wrn_msg += "amortization frequency provided for cap / floor with zero amortization amount;";
                }
                cap_flr.amort_freq = "";

                if (first_amort_date.compare("") != 0) // amortization frequency provided 
                {
                    cap_flr.wrn_msg += "first amortization date provided for cap / floor with zero amortization amount;";
                }
            }
            else // amortization amount provided
            {
                cap_flr.amort = stod(amort);

                if (amort_freq.compare("") == 0) // amortization frequency not provided
                {
                    cap_flr.wrn_msg += "amortization frequency not provided for cap / floor with non-zero amortization amount;";
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
            cap_flr.crv_fwd = rslt->tbl.values[cap_flr_idx][28];

        // perform other sanity checks
       
            // value date
            if (cap_flr.value_date.get_days_no() > cap_flr.maturity_date.get_days_no())
            {
                cap_flr.wrn_msg += "value date cannot by greater than maturity date;";
                cap_flr.value_date = cap_flr.maturity_date;
            }

            // first interest payment date
            if (cap_flr.first_int_date.get_days_no() <= cap_flr.value_date.get_days_no())
            {
                cap_flr.wrn_msg += "first interest payment date cannot be lower or equal to value date;";
                cap_flr.first_int_date = cap_flr.value_date;
                cap_flr.first_int_date.add(cap_flr.int_freq);
                while (cap_flr.first_int_date.get_days_no() < cap_flr.value_date.get_days_no())
                {
                    cap_flr.first_int_date.add(cap_flr.int_freq);
                }
            }

            if (cap_flr.first_int_date.get_days_no() > cap_flr.maturity_date.get_days_no())
            {
                cap_flr.wrn_msg += "first interest payment date cannot be greater than maturity date;";
                cap_flr.first_int_date = cap_flr.maturity_date;
            }

            // first fixing date
            if (cap_flr.first_fix_date.get_days_no() <= cap_flr.value_date.get_days_no())
            {
                cap_flr.wrn_msg += "first fixing date cannot be lower or equal to value date;";
                cap_flr.first_fix_date = cap_flr.first_int_date;
            }

            if (cap_flr.first_fix_date.get_days_no() > cap_flr.maturity_date.get_days_no())
            {
                cap_flr.wrn_msg += "first fixing date cannot be greater than maturity date;";
                cap_flr.first_fix_date = cap_flr.first_int_date;
            }

            if (cap_flr.first_fix_date.get_days_no() < cap_flr.first_int_date.get_days_no())
            {
                cap_flr.wrn_msg += "first fixing date cannot be lower than the first interest payment date;";
                cap_flr.first_fix_date = cap_flr.first_int_date;
            }

            // first amortization date
            if (cap_flr.amort != 0)
            {
                if (cap_flr.first_amort_date.get_days_no() <= cap_flr.value_date.get_days_no())
                {
                    cap_flr.wrn_msg += "first amortization date cannot be lower or equal to value date;";
                    cap_flr.first_amort_date = cap_flr.first_int_date;
                }

                if (cap_flr.first_amort_date.get_days_no() > cap_flr.maturity_date.get_days_no())
                {
                    cap_flr.wrn_msg += "first amortization date cannot be greater than maturity date;";
                    cap_flr.first_amort_date = cap_flr.first_int_date;
                }

                if (cap_flr.first_amort_date.get_days_no() <= cap_flr.first_int_date.get_days_no())
                {
                    cap_flr.wrn_msg += "first amortization date cannot be lower or equal to the first interest payment date;";
                    cap_flr.first_amort_date = cap_flr.first_int_date;
                }
            }

        // check frequencies
        
            // amortization frequency
            if (cap_flr.amort_freq.compare("") != 0)
            {
                if (eval_freq(cap_flr.amort_freq) < eval_freq(cap_flr.int_freq))
                {
                    cap_flr.wrn_msg += "amortization frequency cannot be lower than interest payment frequency;";
                    cap_flr.amort_freq = cap_flr.int_freq;
                }
            }

            // repricing frequency
            if (cap_flr.fix_freq.compare("") != 0)
            {
                if (eval_freq(cap_flr.fix_freq) < eval_freq(cap_flr.int_freq))
                {
                    cap_flr.wrn_msg += "repricing frequency cannot be lower than intrest payment frequency;";
                    cap_flr.fix_freq = cap_flr.int_freq;
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

            // find date of the first interest payment and a create vector interest events;
            // we assume that all other events occur on interest payment dates

            // iterate until you find the first interest payment period that does not preceed calculation date
            date1 = cap_flr.first_int_date;
            date2 = cap_flr.first_int_date;
            date2.remove(cap_flr.int_freq);
            while ((date1.get_days_no() < calc_date.get_days_no()) & (date2.get_days_no() < calc_date.get_days_no()))
            {
                date1 = date2;
                date2.add(cap_flr.int_freq);
            }
            
            // date1 represents the beginning date of such coupon period => we generate coupon dates till maturity
            event_dates = create_date_serie(date1.get_date_str(), cap_flr.maturity_date.get_date_str(), cap_flr.int_freq, date_format);     

            // create a vector of events based on coupon dates
            for (int idx = 0; idx < event_dates.size(); idx++)
            {
                date1 = event_dates[idx];
                date2 = date1;
                date1.remove(cap_flr.int_freq);
                evnt.date_begin = date1; // beging of coupon period
                evnt.date_end = date2; // end of coupon period => date of coupon payment
                evnt.int_year_frac = day_count_method(date1, date2, cap_flr.dcm);

                // indicate that interest payment is fixed and therefore
                // could be calculated without a scenario knowledge
                if (evnt.date_begin.get_days_no() < cap_flr.first_fix_date.get_days_no())
                {
                    evnt.is_int_fixed = true;
                }
                // interest payments in future
                else
                {
                    evnt.is_int_fixed = false;
                }

                events.emplace_back(evnt);
            }

            // extract begin and end dates of the interest payment periods
            begin_int_dates = extract_dates_from_events(events, "date_begin");
            end_int_dates = extract_dates_from_events(events, "date_end");

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
                    // we assign amortization flag based on begining interest payment date
                    if (event_dates[idx].get_date_int() < cap_flr.maturity_date.get_date_int())
                    {
                        pos_idx = get_nearest_idx(event_dates[idx], end_int_dates, "abs");
                        events[pos_idx].amort_flg = true;
                    }
                }
            }

            // add nominal information; we assume that cap_flr.nominal contains the current nominal (i.e. taking potential previous amortizations
            // into account)
            for (int idx = 0; idx < events.size(); idx++)
            {
                // set amortization payment to zero by default
                events[idx].amort_payment = 0.0;

                // set nominal
                if (idx == 0) // first interest payment date => use the current nominal
                {
                    events[idx].nominal_begin = cap_flr.nominal;
                    events[idx].nominal_end = cap_flr.nominal;
                }
                else // on the following interest payment dates we use nominal from the previous interest payment date
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

                // the last interest payment is accompanied by full nominal repayment
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

                // determine in which interest payment periods there will be repricing
                for (int idx = 0; idx < event_dates.size(); idx++)
                {
                    // we assign repricing flag based on begining interest payment date
                    if (event_dates[idx].get_date_int() < cap_flr.maturity_date.get_date_int())
                    {
                        pos_idx = get_nearest_idx(event_dates[idx], begin_int_dates, "abs");
                        events[pos_idx].fix_flg = true;
                    }
                }

                // repricing dates
                for (int idx = 0; idx < events.size(); idx++)
                {
                    // interest rate was fixed in the previous interest payment period; could occur if
                    // e.g interest payment period is 6M and repricing period is 1Y
                    if (!events[idx].is_int_fixed && !events[idx].fix_flg)
                    {
                        events[idx].int_rate = events[idx - 1].int_rate;
                    }
                    // determine interest rate
                    else if (!events[idx].is_int_fixed && events[idx].fix_flg)
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
                        // dates but also in between interest payment dates
                        else if (cap_flr.fix_type.compare("par") == 0)
                        {
                            for (int idx2 = 0; idx2 < events.size(); idx2++)
                            {
                                // check that interest payment date falls within
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

            // add events into structure
            cap_flr.events = events;

            // add cap / floor to vector of caps / floors
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
void myCapsFloors::calc_npv(const int &scn_no, const myCurves &crvs, const myFx &fx, const myVolSurfaces &vol_surfs, const std::string &ref_ccy_nm)
{
    // variables 
    std::vector<std::tuple<int, int>> scn_tenors;

    // go intrument by intrument
    for (int cap_flr_idx = 0; cap_flr_idx < this->info.size(); cap_flr_idx++)
    {
        // go event by event
        for (int idx = 0; idx < this->info[cap_flr_idx].events.size(); idx++)
        {
            // interest rate was already fixed in the previous interest payment period
            if (this->info[cap_flr_idx].events[idx].is_int_fixed | !this->info[cap_flr_idx].events[idx].fix_flg)
            {
                // the first event => no caplet / floorlet
                if (idx == 0)
                {
                    this->info[cap_flr_idx].events[idx].int_rate = 0.0;
                    this->info[cap_flr_idx].events[idx].opt_mat = 0.0;
                    this->info[cap_flr_idx].events[idx].caplet_vol = -1.0l;
                    this->info[cap_flr_idx].events[idx].caplet_d = 0.0;
                    this->info[cap_flr_idx].events[idx].caplet_n = 0.0;
                    this->info[cap_flr_idx].events[idx].caplet_N = 0.0;
                    this->info[cap_flr_idx].events[idx].floorlet_vol = -1.0;
                    this->info[cap_flr_idx].events[idx].floorlet_d = 0.0;
                    this->info[cap_flr_idx].events[idx].floorlet_n = 0.0;
                    this->info[cap_flr_idx].events[idx].floorlet_N = 0.0;
                }
                // other events => take values from the previous interest payment period
                else
                {
                    this->info[cap_flr_idx].events[idx].int_rate = this->info[cap_flr_idx].events[idx - 1].int_rate;
                    this->info[cap_flr_idx].events[idx].opt_mat = this->info[cap_flr_idx].events[idx - 1].opt_mat;
                    this->info[cap_flr_idx].events[idx].caplet_vol = this->info[cap_flr_idx].events[idx - 1].caplet_vol;
                    this->info[cap_flr_idx].events[idx].caplet_d = this->info[cap_flr_idx].events[idx - 1].caplet_d;
                    this->info[cap_flr_idx].events[idx].caplet_n = this->info[cap_flr_idx].events[idx - 1].caplet_n;
                    this->info[cap_flr_idx].events[idx].caplet_N = this->info[cap_flr_idx].events[idx - 1].caplet_N;
                    this->info[cap_flr_idx].events[idx].floorlet_vol = this->info[cap_flr_idx].events[idx - 1].floorlet_vol;
                    this->info[cap_flr_idx].events[idx].floorlet_d = this->info[cap_flr_idx].events[idx - 1].floorlet_d;
                    this->info[cap_flr_idx].events[idx].floorlet_n = this->info[cap_flr_idx].events[idx - 1].floorlet_n;
                    this->info[cap_flr_idx].events[idx].floorlet_N = this->info[cap_flr_idx].events[idx - 1].floorlet_N;
                }
            }
            // calculate forward rate / par rate and determine interest rate volatility
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
                    this->info[cap_flr_idx].events[idx].int_rate = fwds[0];
                }
                // apply par-rate
                else
                {
                    // get par-rate
                    int par_step = this->info[cap_flr_idx].events[idx].repricing_dates.size() - 1;
                    std::vector<double> pars = crvs.get_par_rate(this->info[cap_flr_idx].crv_fwd, scn_tenors, this->info[cap_flr_idx].events[idx].par_nominals_begin, this->info[cap_flr_idx].events[idx].par_nominals_end, par_step, this->info[cap_flr_idx].dcm);

                    // user the par-rate as a coupon rate
                    this->info[cap_flr_idx].events[idx].int_rate = pars[0];
                }

                // caplet / floorlet maturity date
                myDate maturity = this->info[cap_flr_idx].events[idx].date_begin;
                maturity.add(this->info[cap_flr_idx].fix_freq);
                this->info[cap_flr_idx].events[idx].opt_mat = (maturity.get_days_no() - this->info[cap_flr_idx].calc_date.get_days_no()) / 365.;

                // volatility tenor
                std::vector<double> tenors = {static_cast<double>(maturity.get_days_no() - this->info[cap_flr_idx].calc_date.get_days_no())};

                // caplet / floorlet execution date
                double execution = (this->info[cap_flr_idx].events[idx].date_begin.get_days_no() - this->info[cap_flr_idx].calc_date.get_days_no()) / 365.;

                // get interest rate volatility for caplets and calculation d parameter of the normal model
                std::string cap_vol_surf_nm = this->info[cap_flr_idx].cap_vol_surf;
                if (cap_vol_surf_nm.compare("") != 0)
                {
                    // get caplet volatility
                    std::vector<double> caplet_vols = vol_surfs.get_vols(cap_vol_surf_nm, scn_no, tenors, {this->info[cap_flr_idx].cap_rate});
                    this->info[cap_flr_idx].events[idx].caplet_vol = caplet_vols[0];

                    // calculate d
                    this->info[cap_flr_idx].events[idx].caplet_d = (this->info[cap_flr_idx].events[idx].int_rate - this->info[cap_flr_idx].cap_rate) / (this->info[cap_flr_idx].events[idx].caplet_vol * std::sqrt(execution));
                    this->info[cap_flr_idx].events[idx].caplet_n = norm_pdf({this->info[cap_flr_idx].events[idx].caplet_d})[0];
                    this->info[cap_flr_idx].events[idx].caplet_N = norm_cdf({this->info[cap_flr_idx].events[idx].caplet_d})[0];
                }
                else
                {
                    this->info[cap_flr_idx].events[idx].caplet_vol = -1.0;
                    this->info[cap_flr_idx].events[idx].caplet_d = 0.0;
                }

                // get interest rate volatility for floorlets and calculation d parameter of the normal model
                std::string floor_vol_surf_nm = this->info[cap_flr_idx].floor_vol_surf;
                if (floor_vol_surf_nm.compare("") != 0)
                {
                    // get floorlet volatility
                    std::vector<double> floorlet_vols = vol_surfs.get_vols(floor_vol_surf_nm, scn_no, tenors, {this->info[cap_flr_idx].floor_rate});
                    this->info[cap_flr_idx].events[idx].floorlet_vol = floorlet_vols[0];
                
                    // calculate d
                    this->info[cap_flr_idx].events[idx].floorlet_d = (this->info[cap_flr_idx].events[idx].int_rate - this->info[cap_flr_idx].floor_rate) / (this->info[cap_flr_idx].events[idx].floorlet_vol * std::sqrt(execution));
                    this->info[cap_flr_idx].events[idx].floorlet_n = norm_pdf({this->info[cap_flr_idx].events[idx].floorlet_d})[0];
                    this->info[cap_flr_idx].events[idx].floorlet_N = norm_cdf({-this->info[cap_flr_idx].events[idx].floorlet_d})[0];
                }
                else
                {
                    this->info[cap_flr_idx].events[idx].floorlet_vol = -1.0;
                    this->info[cap_flr_idx].events[idx].floorlet_d = 0.0;
                }
            }
        }

        // calculate discount factors and total NPV
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

        // calculate NPV of caplets / floorlets
        for (int idx = 0; idx < this->info[cap_flr_idx].events.size(); idx++)
        {
            // prepare variables
            double f = this->info[cap_flr_idx].events[idx].int_rate;
            double k_cap = this->info[cap_flr_idx].cap_rate;
            double k_floor = this->info[cap_flr_idx].floor_rate;
            double T = this->info[cap_flr_idx].events[idx].opt_mat;
            double nominal = this->info[cap_flr_idx].events[idx].nominal_begin;
            double dt = this->info[cap_flr_idx].events[idx].int_year_frac;
            double df = this->info[cap_flr_idx].events[idx].df;
            double caplet_vol = this->info[cap_flr_idx].events[idx].caplet_vol;
            double caplet_n = this->info[cap_flr_idx].events[idx].caplet_n;
            double caplet_N = this->info[cap_flr_idx].events[idx].caplet_N;
            double floorlet_vol = this->info[cap_flr_idx].events[idx].floorlet_vol;
            double floorlet_n = this->info[cap_flr_idx].events[idx].floorlet_n;
            double floorlet_N = this->info[cap_flr_idx].events[idx].floorlet_N;
            
            // caplet
            if (caplet_vol >= 0)
            {
                double npv =  nominal * dt * df * ((f - k_cap) * caplet_N + caplet_vol * std::sqrt(T) * caplet_n);
                this->info[cap_flr_idx].events[idx].caplet_npv = npv;
            }
            else
            {
                this->info[cap_flr_idx].events[idx].caplet_npv = 0.0;
            }

            // floorlet
            if (floorlet_vol >= 0)
            {
                double npv = nominal * dt * df * ((k_floor - f) * floorlet_N + floorlet_vol * std::sqrt(T) * floorlet_n);
                this->info[cap_flr_idx].events[idx].floorlet_npv = npv;
            }
            else
            {
                this->info[cap_flr_idx].events[idx].floorlet_npv = 0.0;
            }

            // update NPV
            this->info[cap_flr_idx].cap_npv += this->info[cap_flr_idx].events[idx].caplet_npv;
            this->info[cap_flr_idx].floor_npv += this->info[cap_flr_idx].events[idx].floorlet_npv;
            this->info[cap_flr_idx].tot_npv += (this->info[cap_flr_idx].events[idx].caplet_npv + this->info[cap_flr_idx].events[idx].floorlet_npv);
        }

        // calculate NPV in reference currency
        std::tuple<int, std::string> scn_ccy = std::tuple<int, std::string>(scn_no, ref_ccy_nm);
        double fx_rate = fx.get_fx(scn_ccy);
        this->info[cap_flr_idx].cap_npv_ref_ccy = fx_rate * this->info[cap_flr_idx].cap_npv;
        this->info[cap_flr_idx].floor_npv_ref_ccy = fx_rate * this->info[cap_flr_idx].floor_npv;
        this->info[cap_flr_idx].tot_npv_ref_ccy = fx_rate * this->info[cap_flr_idx].tot_npv;
    }
}

// calculate NPV using multithreading
std::thread myCapsFloors::calc_npv_thrd(const int &scn_no, const myCurves &crvs, const myFx &fx, const myVolSurfaces &vol_surfs, const std::string &ref_ccy_nm)
{
    std::thread worker(&myCapsFloors::calc_npv, this, scn_no, crvs, fx, vol_surfs, ref_ccy_nm);
    return worker;
}

// write NPV into SQLite database file
void myCapsFloors::write_npv(mySQLite &db, const int &scn_no, const std::string &ent_nm, const std::string &ptf)
{
    // initiate dataframe structure
    dataFrame df;

    // add column names
    df.col_nms = {"scn_no", "ent_nm", "parent_id", "contract_id", "ptf", "cap_npv", "cap_npv_ref_ccy", "floor_npv", "floor_npv_ref_ccy", "tot_npv", "tot_npv_ref_ccy"};

    // add column data type
    df.dtypes = {"INT", "CHAR", "CHAR", "CHAR", "CHAR", "FLOAT", "FLOAT", "FLOAT", "FLOAT", "FLOAT", "FLOAT"};
    
    // add values
    std::vector<std::vector<std::string>> values;
    std::vector<std::string> cap_flr;

    // go instrument by instrument
    for (int cap_flr_idx = 0; cap_flr_idx < this->info.size(); cap_flr_idx++)
    {
        cap_flr.clear();
        cap_flr.push_back(std::to_string(scn_no));
        cap_flr.push_back(this->info[cap_flr_idx].ent_nm);
        cap_flr.push_back(this->info[cap_flr_idx].parent_id);
        cap_flr.push_back(this->info[cap_flr_idx].contract_id);
        cap_flr.push_back(this->info[cap_flr_idx].ptf);
        cap_flr.push_back(std::to_string(this->info[cap_flr_idx].cap_npv));
        cap_flr.push_back(std::to_string(this->info[cap_flr_idx].cap_npv_ref_ccy));
        cap_flr.push_back(std::to_string(this->info[cap_flr_idx].floor_npv));
        cap_flr.push_back(std::to_string(this->info[cap_flr_idx].floor_npv_ref_ccy));
        cap_flr.push_back(std::to_string(this->info[cap_flr_idx].tot_npv));
        cap_flr.push_back(std::to_string(this->info[cap_flr_idx].tot_npv_ref_ccy));
        values.push_back(cap_flr);
    }
    df.values = values;

    // create dataframe object
    myDataFrame tbl = myDataFrame(df);

    // delete old data based on scenario number, entity name and portfolio
    std::string sql = "DELETE FROM cap_floor_npv WHERE scn_no = " + std::to_string(scn_no) + " AND ent_nm = '" + ent_nm + "' AND ptf = '" + ptf + "';";
    db.exec(sql);

    // write dataframe into SQLite database file
    db.upload_tbl(tbl, "cap_floor_npv", false);
}
