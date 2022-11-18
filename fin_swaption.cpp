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
#include "fin_swaption.h"

/*
 * AUXILIARY FUNCTIONS
 */

// extract vector of dates from vector of events
static std::vector<myDate> extract_dates_from_events(const std::vector<swpt_event> &events, const std::string &type)
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

mySwaptions::mySwaptions(const mySQLite &db, const std::string &sql, const myDate &calc_date)
{
    // dataframe to hold result of SQL query
    myDataFrame * rslt = new myDataFrame();

    // load bond portfolio as specified by SQL query
    rslt = db.query(sql);

    // auxiliary variables
    std::string aux;
    std::vector<myDate> int_event_dates;
    std::vector<myDate> amort_event_dates;
    std::vector<myDate> end_int_dates;

    // reserve memory to avoid memory resize
    this->info.reserve(rslt->tbl.values.size());

    // go bond by bond
    for (int swpt_idx = 0; swpt_idx < rslt->tbl.values.size(); swpt_idx++)
    {
        // load bond information and perform some sanity checks
        swpt_info swpt;

            // entity name
            swpt.ent_nm = rslt->tbl.values[swpt_idx][0];

            // parent id; useful to bind two or more bonds together to create a new product like IRS
            swpt.parent_id = rslt->tbl.values[swpt_idx][1];

            // contract id
            swpt.contract_id = rslt->tbl.values[swpt_idx][2];

            // issuer id
            swpt.issuer_id = rslt->tbl.values[swpt_idx][3];

            // portfolio
            swpt.ptf = rslt->tbl.values[swpt_idx][4];

            // account
            swpt.account = rslt->tbl.values[swpt_idx][5];

            // swaption ISIN
            swpt.isin = rslt->tbl.values[swpt_idx][6];

            // rating
            swpt.rtg = rslt->tbl.values[swpt_idx][7];

            // comments
            swpt.comments = rslt->tbl.values[swpt_idx][8];

            // swaption type - call / put
            swpt.swaption_type = rslt->tbl.values[swpt_idx][9];

            // currency, e.g. EUR, CZK
            swpt.ccy_nm = rslt->tbl.values[swpt_idx][10];

            // nominal
            swpt.nominal = stod(rslt->tbl.values[swpt_idx][11]);

            // calculation date
            swpt.calc_date = calc_date;

            // value date
            swpt.value_date = myDate(stoi(rslt->tbl.values[swpt_idx][12]));

            // maturity date
            swpt.maturity_date = myDate(stoi(rslt->tbl.values[swpt_idx][13]));

            // day count method used to calculate interent payment
            swpt.dcm = rslt->tbl.values[swpt_idx][14];
            
            // swaption rate
            aux = rslt->tbl.values[swpt_idx][15];
            if (aux.compare("") == 0)
            {
                swpt.swaption_rate = 100;
                swpt.wrn_msg += "missing swaption rate;";
            }
            else
            {
                swpt.swaption_rate = stod(aux);
            }
            
            // swaption volatility surface
            swpt.swaption_vol_surf = rslt->tbl.values[swpt_idx][16];
            
            // fixing frequency of the underlying swap
            swpt.fix_freq = rslt->tbl.values[swpt_idx][17];

            // amortization
            std::string first_amort_date = rslt->tbl.values[swpt_idx][18];
            std::string amort_freq = rslt->tbl.values[swpt_idx][19];
            std::string amort = rslt->tbl.values[swpt_idx][20];
            
            if (amort.compare("") == 0) // amortization amount not provided
            {
                swpt.amort = 0.0;

                if (amort_freq.compare("") != 0) // amortization frequency provided
                {
                    swpt.wrn_msg += "amortization frequency provided for cap / floor with zero amortization amount;";
                }
                swpt.amort_freq = "";

                if (first_amort_date.compare("") != 0) // amortization frequency provided 
                {
                    swpt.wrn_msg += "first amortization date provided for cap / floor with zero amortization amount;";
                }
            }
            else // amortization amount provided
            {
                swpt.amort = stod(amort);

                if (amort_freq.compare("") == 0) // amortization frequency not provided
                {
                    swpt.wrn_msg += "amortization frequency not provided for cap / floor with non-zero amortization amount;";
                    swpt.amort = 0.0;
                    swpt.amort_freq = "";
                }
                else // amortization frequency provided
                {
                    swpt.amort_freq = amort_freq;
                }

                if (first_amort_date.compare("") == 0) // first amortization date not provided
                {
                    swpt.wrn_msg += "first amortization date not provided for a bond with non-zero amortization amount;";
                    swpt.amort = 0.0;
                    swpt.amort_freq = "";
                }
                else // first amortization date provided
                {
                    swpt.first_amort_date = myDate(stoi(first_amort_date));
                }
            }
            
            // discounting curve
            swpt.crv_disc = rslt->tbl.values[swpt_idx][21];

            // repricing curve
            swpt.crv_fwd = rslt->tbl.values[swpt_idx][22];

        // perform other sanity checks
       
            // value date
            if (swpt.value_date.get_days_no() > swpt.maturity_date.get_days_no())
            {
                swpt.wrn_msg += "value date cannot by greater than maturity date;";
                swpt.value_date = swpt.maturity_date;
            }

            // date of the first interest payment => anchor date for the first amortization date
            myDate first_int_date = swpt.value_date;
            first_int_date.add(swpt.fix_freq);

            // first amortization date
            if (swpt.amort != 0)
            {
                if (swpt.first_amort_date.get_days_no() <= swpt.value_date.get_days_no())
                {
                    swpt.wrn_msg += "first amortization date cannot be lower or equal to value date;";
                    swpt.first_amort_date = first_int_date;
                }

                if (swpt.first_amort_date.get_days_no() > swpt.maturity_date.get_days_no())
                {
                    swpt.wrn_msg += "first amortization date cannot be greater than maturity date;";
                    swpt.first_amort_date = first_int_date;
                }
            }

        // check frequencies
        
            // amortization frequency
            if (swpt.amort_freq.compare("") != 0)
            {
                if (eval_freq(swpt.amort_freq) < eval_freq(swpt.fix_freq))
                {
                    swpt.wrn_msg += "amortization frequency cannot be lower than interest payment frequency;";
                    swpt.amort_freq = swpt.fix_freq;
                }
            }

        // generate vector of events

            // variables containing dates
            myDate date1;
            myDate date2;

            // events
            swpt_event evnt;
            std::vector<swpt_event> events;

            // position index
            int pos_idx;

            // date format, which is used to store date string in object myDate
            std::string date_format = "yyyymmdd";

            // create vector interest events; we assume that all other events occur on interest payment dates
            int_event_dates = create_date_serie(swpt.value_date.get_date_str(), swpt.maturity_date.get_date_str(), swpt.fix_freq, date_format);     

            // create a vector of events based on interest payment dates
            for (int idx = 0; idx < int_event_dates.size() - 1; idx++)
            {
                evnt.date_begin = int_event_dates[idx];
                evnt.date_end = int_event_dates[idx + 1];
                evnt.int_year_frac = day_count_method(evnt.date_begin, evnt.date_end, swpt.dcm);
                events.emplace_back(evnt);
            }

            // add amortization information to events vector
            if (swpt.amort_freq.compare("") != 0)
            {
                // iterate until you find the first amortization date after calculation date
                date1 = swpt.first_amort_date;
                while (date1.get_days_no() < calc_date.get_days_no())
                {
                    date1.add(swpt.amort_freq);
                }

                // date1 is the first amortization date => we generate amortizaton dates till maturity
                amort_event_dates = create_date_serie(date1.get_date_str(), swpt.maturity_date.get_date_str(), swpt.amort_freq, date_format);

                // extract begin and end dates of the coupon periods
                end_int_dates = extract_dates_from_events(events, "date_end");


                // go amortization date by amortization date and assign it the nearest coupon date; we assume that amortizations happen on coupon dates
                for (int idx = 0; idx < amort_event_dates.size(); idx++)
                {
                    // we assign amortization flag based on begining interest payment date
                    if (amort_event_dates[idx].get_date_int() < swpt.maturity_date.get_date_int())
                    {
                        pos_idx = get_nearest_idx(amort_event_dates[idx], end_int_dates, "abs");
                        events[pos_idx].amort_flg = true;
                    }
                }
            }

            // add nominal information; we assume that swpt.nominal contains the current nominal (i.e. taking potential previous amortizations
            // into account)
            for (int idx = 0; idx < events.size(); idx++)
            {
                // set amortization payment to zero by default
                events[idx].amort_payment = 0.0;

                // set nominal
                if (idx == 0) // first interest payment date => use the current nominal
                {
                    events[idx].nominal_begin = swpt.nominal;
                    events[idx].nominal_end = swpt.nominal;
                }
                else // on the following interest payment dates we use nominal from the previous interest payment date
                {
                    events[idx].nominal_begin = events[idx - 1].nominal_end;
                    events[idx].nominal_end = events[idx - 1].nominal_end;
                }

                // adjust for amortization payment
                if (events[idx].amort_flg)
                {
                    events[idx].nominal_end -= swpt.amort;
                    events[idx].amort_payment = swpt.amort;
                }

                // the last interest payment is accompanied by full nominal repayment
                if (idx == events.size() - 1)
                {
                    events[idx].amort_payment = events[idx].nominal_begin;
                    events[idx].nominal_end = 0.0;
                }
            }

            // add events into structure
            swpt.events = events;

            // add cap / floor to vector of caps / floors
            this->info.emplace_back(swpt);
    
        }

    // delete unused pointers
    delete rslt;
}

/*
 * OBJECT FUNCTIONS
 */

// split the object
std::vector<mySwaptions> mySwaptions::split(const int &threads_no)
{

    // determine position indicies of the newly created vectors
    std::vector<coordinates<int>> indicies = split_vector(this->info.size(), threads_no);

    // split original object myAnnuities into several smaller objects
    std::vector<mySwaptions> swpts;
    for (int thread_idx = 0; thread_idx < threads_no; thread_idx++)
    {
        // determine the first and the last position index
        std::vector<swpt_info>::const_iterator first = this->info.begin() + indicies[thread_idx].x;
        std::vector<swpt_info>::const_iterator last = this->info.begin() + indicies[thread_idx].y + 1;

        // extract bond information
        std::vector<swpt_info> info(first, last);

        // create new smaller object based on the position indicies
        mySwaptions swpt(info);

        // add the new object to the vector
        swpts.emplace_back(swpt);
    }

    // release memory by clearing the original object
    this->clear();

    // return vector of objects
    return swpts;
}

// merge several objects
void mySwaptions::merge(std::vector<mySwaptions> &swpts)
{
    // pre-allocate memory
    int final_vector_size = 0;
    std::vector<swpt_info> info;
    
    for (int idx = 0; idx < swpts.size(); idx++)
    {
        final_vector_size += swpts[idx].info.size();
    }

    info.reserve(final_vector_size);

    // merge individual vectors
    for (int idx = 0; idx < swpts.size(); idx++)
    {
        info.insert(info.end(), swpts[idx].info.begin(), swpts[idx].info.end());
        swpts[idx].clear(); // release memory by clearing individual objects
    }

    // copy merged vectors into the current object
    this->info = info;
}

// calculate NPV
void mySwaptions::calc_npv(const int &scn_no, const myCurves &crvs, const myFx &fx, const myVolSurfaces &vol_surfs, const std::string &ref_ccy_nm)
{
    // variables 
    std::vector<std::tuple<int, int>> scn_tenors;
    double aux1;
    double aux2;

    // go intrument by intrument
    for (int swpt_idx = 0; swpt_idx < this->info.size(); swpt_idx++)
    {
        // clear selected fields
        this->info[swpt_idx].aux1 = 0.0;
        this->info[swpt_idx].aux2 = 0.0;
        this->info[swpt_idx].aux3 = 0.0;
        this->info[swpt_idx].aux4 = 0.0;
        this->info[swpt_idx].flt_leg_npv = 0.0;

        // go event by event
        for (int idx = 0; idx < this->info[swpt_idx].events.size(); idx++)
        {
            // calculate forward rate
            scn_tenors.clear();
            scn_tenors.push_back(std::tuple<int, int>(scn_no, this->info[swpt_idx].events[idx].date_begin.get_date_int()));
            scn_tenors.push_back(std::tuple<int, int>(scn_no, this->info[swpt_idx].events[idx].date_end.get_date_int()));
            this->info[swpt_idx].events[idx].fwd = crvs.get_fwd_rate(this->info[swpt_idx].crv_fwd, scn_tenors, this->info[swpt_idx].dcm)[0];

            // calculate discount factor
            scn_tenors.clear();
            scn_tenors.push_back(std::tuple<int, int>(scn_no, this->info[swpt_idx].events[idx].date_end.get_date_int()));
            this->info[swpt_idx].events[idx].df = crvs.get_df(this->info[swpt_idx].crv_disc, scn_tenors)[0];

            // calculate NPV components
            aux1 = this->info[swpt_idx].events[idx].nominal_begin * this->info[swpt_idx].events[idx].int_year_frac * this->info[swpt_idx].events[idx].df;
            aux2 = this->info[swpt_idx].events[idx].amort_payment * this->info[swpt_idx].events[idx].df;
            this->info[swpt_idx].aux1 += aux1;
            this->info[swpt_idx].aux2 += aux2;
            this->info[swpt_idx].flt_leg_npv += aux1 * this->info[swpt_idx].events[idx].fwd + aux2;
        }

        // determine rate of the underlying swap
        this->info[swpt_idx].swap_rate = (this->info[swpt_idx].flt_leg_npv - this->info[swpt_idx].aux2) / this->info[swpt_idx].aux1;

        // get volatility
        std::vector<double> tenors = {static_cast<double>(this->info[swpt_idx].value_date.get_days_no() - this->info[swpt_idx].calc_date.get_days_no())};
        this->info[swpt_idx].swaption_vol = vol_surfs.get_vols(this->info[swpt_idx].swaption_vol_surf, scn_no, tenors, {this->info[swpt_idx].swaption_rate})[0];

        // calculate swaption maturity
        double swpt_mat = day_count_method(this->info[swpt_idx].calc_date, this->info[swpt_idx].value_date, this->info[swpt_idx].dcm);

        // calculation swaption price
        this->info[swpt_idx].d = (this->info[swpt_idx].swap_rate - this->info[swpt_idx].swaption_rate) / (this->info[swpt_idx].swaption_vol * std::sqrt(swpt_mat));
        if (this->info[swpt_idx].swaption_type.compare("call") == 0)
        {
            this->info[swpt_idx].aux3 = 1.0;
        }
        else
        {
            this->info[swpt_idx].aux3 = -1.0;
        }
        this->info[swpt_idx].aux4 = this->info[swpt_idx].aux3 * (this->info[swpt_idx].swap_rate - this->info[swpt_idx].swaption_rate) * norm_cdf({this->info[swpt_idx].aux3 * this->info[swpt_idx].d})[0];
        this->info[swpt_idx].npv = (this->info[swpt_idx].aux4 + this->info[swpt_idx].swaption_vol * std::sqrt(swpt_mat) * norm_pdf({this->info[swpt_idx].d})[0]) * this->info[swpt_idx].aux1;

        // calculate NPV in reference currency
        std::tuple<int, std::string> scn_ccy = std::tuple<int, std::string>(scn_no, ref_ccy_nm);
        double fx_rate = fx.get_fx(scn_ccy);
        this->info[swpt_idx].npv_ref_ccy = fx_rate * this->info[swpt_idx].npv;
    }
}

// calculate NPV using multithreading
std::thread mySwaptions::calc_npv_thrd(const int &scn_no, const myCurves &crvs, const myFx &fx, const myVolSurfaces &vol_surfs, const std::string &ref_ccy_nm)
{
    std::thread worker(&mySwaptions::calc_npv, this, scn_no, crvs, fx, vol_surfs, ref_ccy_nm);
    return worker;
}

// write NPV into SQLite database file
void mySwaptions::write_npv(mySQLite &db, const int &scn_no, const std::string &ent_nm, const std::string &ptf)
{
    // initiate dataframe structure
    dataFrame df;

    // add column names
    df.col_nms = {"scn_no", "ent_nm", "parent_id", "contract_id", "ptf", "npv", "npv_ref_ccy"};

    // add column data type
    df.dtypes = {"INT", "CHAR", "CHAR", "CHAR", "CHAR", "FLOAT", "FLOAT"};
    
    // add values
    std::vector<std::vector<std::string>> values;
    std::vector<std::string> swpt;

    // go instrument by instrument
    for (int swpt_idx = 0; swpt_idx < this->info.size(); swpt_idx++)
    {
        swpt.clear();
        swpt.push_back(std::to_string(scn_no));
        swpt.push_back(this->info[swpt_idx].ent_nm);
        swpt.push_back(this->info[swpt_idx].parent_id);
        swpt.push_back(this->info[swpt_idx].contract_id);
        swpt.push_back(this->info[swpt_idx].ptf);
        swpt.push_back(std::to_string(this->info[swpt_idx].npv));
        swpt.push_back(std::to_string(this->info[swpt_idx].npv_ref_ccy));
        values.push_back(swpt);
    }
    df.values = values;

    // create dataframe object
    myDataFrame tbl = myDataFrame(df);

    // delete old data based on scenario number, entity name and portfolio
    std::string sql = "DELETE FROM swaption_npv WHERE scn_no = " + std::to_string(scn_no) + " AND ent_nm = '" + ent_nm + "' AND ptf = '" + ptf + "';";
    db.exec(sql);

    // write dataframe into SQLite database file
    db.upload_tbl(tbl, "swaption_npv", false);
}
