#include <string>
#include <iostream>
#include <tuple>
#include <map>
#include <vector>
#include <memory>
#include <math.h>
#include <thread>
#include "lib_aux.h"
#include "lib_date.h"
#include "lib_dataframe.h"
#include "fin_curve.h"
#include "fin_fx.h"
#include "fin_date.h"
#include "fin_annuity.h"

/*
 * AUXILIARY FUNCTIONS
 */

// calculate annuity payment assuming initial nominal of 1 currency unit
static double calc_ann_payment(const double &rate, const int &rmng_ann_payments, const int &payment_freq)
{
   double q = 1 / (1 + rate / payment_freq);
   return 1 / (q * (1 - static_cast<double>(pow(q, rmng_ann_payments)))) * (1 - q);
}

// calculate npv of annuity; this is an auxiliary function used in annuity repricing function
static double calc_ann_npv_aux(const double &rate, const int &rmng_ann_payments, const int &payment_freq, const std::vector<double> &dfs)
{
    // set fictive initial nominal
    double nominal = 1.0;

    // calculate annuity payment
    double ann_payment = calc_ann_payment(rate, rmng_ann_payments, payment_freq) * nominal;

    // variable to hold nominal
    double npv = nominal * dfs[0];

    // get number of annuity payments to evaluate
    int ann_payments_no = dfs.size() - 1;

    // determine end nominal and cumulative discount factor
    double df_cum = 0.0;
    for (int idx = 1; idx <= ann_payments_no; idx++)
    {
        // cumulative discount factor
        df_cum += dfs[idx];

        // determinal nominal amortization
        nominal -= (ann_payment - rate / payment_freq * nominal);
    }

    // adjust npv for annuity payments
    npv -= ann_payment * df_cum;

    // adjust npv for end nominal
    npv -= nominal * dfs[dfs.size() - 1];

    // return npv
    return npv;
}


// calculate annuity repricing rate
static double get_ann_rate(const int &rmng_ann_payments, const int &payment_freq, const std::vector<double> &dfs, const int &max_iter_no, const double &precission)
{
    // get estimate of repricing period
    double dt = (dfs.size() - 1) / payment_freq;

    // calculate forward rate as an initial estimate
    double r;
    if (dt > 1.0)
        r = static_cast<double>(pow(dfs[0] / dfs[dfs.size() - 1], 1 / dt)) - 1;
    else
        r = (dfs[0] / dfs[dfs.size() - 1] - 1) / dt;

    // set delta rate to determine numerical derivation
    double dr = 0.0001;

    // determine optimal rate through Newton-Raphson method
    for (int iter_idx = 0; iter_idx < max_iter_no; iter_idx++)
    {
        // calculate auxiliary npv and check if further iteration is needed
        double npv = calc_ann_npv_aux(r, rmng_ann_payments, payment_freq, dfs);
        if (abs(npv) < precission)
        {
            return r;
        }
     
        // unfortunately, further iteration is needed
        double npv_down = calc_ann_npv_aux(r - dr, rmng_ann_payments, payment_freq, dfs);
        double npv_up = calc_ann_npv_aux(r + dr, rmng_ann_payments, payment_freq, dfs);

        // calculate numerical derivative
        double d_npv = (npv_up - npv_down) / (2 * dr);

        // calculate annuity rate correction if needed
        if (abs(d_npv) > 1e-10)
        {
            r -= npv / d_npv;
        }
        else
        {
            return r;
        }
    }

    // return repricing annuity rate
    return r;
}

// extract vector of dates from vector of events
static std::vector<myDate> extract_dates_from_events(const std::vector<ann_event> &events, const std::string &type)
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

myAnnuities::myAnnuities(const mySQLite &db, const std::string &sql, const myDate &calc_date)
{
    // dataframe to hold result of SQL query
    myDataFrame * rslt = new myDataFrame();

    // load bond portfolio as specified by SQL query
    rslt = db.query(sql);

    // auxiliary variables
    std::string aux;
    myDate date_aux;
    std::vector<myDate> event_dates;
    std::vector<myDate> begin_ann_dates;
    std::vector<myDate> end_ann_dates;

    // reserve memory to avoid memory resize
    this->info.reserve(rslt->tbl.values.size());

    // go annuity by annuity
    for (int ann_idx = 0; ann_idx < rslt->tbl.values.size(); ann_idx++)
    {
        // load bond information and perform some sanity checks
        ann_info ann;

            // entity name
            ann.ent_nm = rslt->tbl.values[ann_idx][0];

            // parent id; useful to bind two or more annuities together to
            // create a new product
            ann.parent_id = rslt->tbl.values[ann_idx][1];

            // contract id
            ann.contract_id = rslt->tbl.values[ann_idx][2];

            // issuer id
            ann.issuer_id = rslt->tbl.values[ann_idx][3];

            // portfolio
            ann.ptf = rslt->tbl.values[ann_idx][4];

            // account
            ann.account = rslt->tbl.values[ann_idx][5];

            // annuity ISIN
            ann.isin = rslt->tbl.values[ann_idx][6];

            // annuity rating
            ann.rtg = rslt->tbl.values[ann_idx][7];

            // comments
            ann.comments = rslt->tbl.values[ann_idx][8];

            // annuity type, e.g. ANN
            ann.ann_type = rslt->tbl.values[ann_idx][9];

            // fixing type - "par" for a floating annuity vs. "fix" for a fixed annuity
            aux = rslt->tbl.values[ann_idx][10];
            if ((aux.compare("par") != 0) && (aux.compare("fix") != 0))
            {
                ann.wrn_msg += "unsupported fixing type " + aux + ";";
                ann.fix_type = "fix";
            }
            
            if (aux.compare("fix") == 0)
            {
                ann.is_fixed = true;
            }
            else
            {
                ann.is_fixed = false;
            }
            ann.fix_type = aux;

            //  annuity currency, e.g. EUR, CZK
            ann.ccy_nm = rslt->tbl.values[ann_idx][11];

            // annuity nominal
            ann.nominal = stod(rslt->tbl.values[ann_idx][12]);

            // value date
            ann.value_date = myDate(stoi(rslt->tbl.values[ann_idx][13]));

            // annuity maturity date
            ann.maturity_date = myDate(stoi(rslt->tbl.values[ann_idx][14]));

            // annuity accrued interest
            aux = rslt->tbl.values[ann_idx][15];
            if (aux.compare("") == 0) // accrued interest not provided
            {
                ann.wrn_msg += "accrued interest not provided;";
                ann.is_acc_int = false;
                ann.ext_acc_int = 0.0;
            }
            else // accrued interest provided
            {
                ann.is_acc_int = true;
                ann.ext_acc_int = stod(aux);
            }

            // internal rate
            aux = rslt->tbl.values[ann_idx][16];
            if (aux.compare("") == 0) // internal rate not provided
            {
                ann.wrn_msg += "internal rate not provided;";
                ann.is_fixed = true;
                ann.is_acc_int = true;
                ann.ext_acc_int = 0.0;
                ann.int_rate = 0.0;
                ann.ann_freq = "";
                ann.fix_type = "fix";
                ann.first_ann_date = ann.maturity_date;
            }
            else // internal rate provided
            {
                ann.int_rate = stod(aux);
            }

            // first annuity date
            ann.first_ann_date =  myDate(stoi(rslt->tbl.values[ann_idx][17]));

            // annuity frequency
            ann.ann_freq = rslt->tbl.values[ann_idx][18];

            if (ann.ann_freq.compare("1M") == 0)
            {
                ann.ann_freq_aux = 12;
            }
            else if (ann.ann_freq.compare("3M") == 0)
            {
                ann.ann_freq_aux = 4;
            }
            else if (ann.ann_freq.compare("6M") == 0)
            {
                ann.ann_freq_aux = 2;
            }
            else if ((ann.ann_freq.compare("12M") == 0) | (ann.ann_freq.compare("1Y") == 0))
            {
                ann.ann_freq_aux = 1;
            }
            else
            {
                throw std::runtime_error ((std::string)__func__ + ": " + ann.ann_freq + " is not supported annuity frequency!");
            }

            // first fixing date of a floating annuity
            aux = rslt->tbl.values[ann_idx][19];
            if (aux.compare("") == 0) // fixing date not provided
            {
                if (!ann.is_fixed) // floating annuity
                {
                    ann.wrn_msg += "first fixing date not provided for a floating annuity;";
                    ann.is_fixed = true;
                    ann.fix_freq = "";
                    ann.fix_type = "fix";
                }
            }
            else // fixing date provided
            {
                if (!ann.is_fixed) // floating annuity
                {
                    ann.first_fix_date = myDate(stoi(aux));
                }
                else // fixed annuity
                {
                    ann.wrn_msg += "first fixing date provided for a fixed annuity;";
                }
            }
            
            // fixing frequency
            aux = rslt->tbl.values[ann_idx][20];
            if (aux.compare("") == 0) // fixing frequency not provided
            {
                if (!ann.is_fixed) // floating annuity
                {
                    ann.wrn_msg += "fixing frequency not provided a floating annuity;";
                    ann.is_fixed = true;
                    ann.fix_freq = "";
                    ann.fix_type = "fix";
                }
                else // fixed annuity
                {
                    ann.fix_freq = "";
                }
            }
            else // fixing frequency provided
            {
                if (!ann.is_fixed) // floating annuity
                {
                    ann.fix_freq = aux;
                }
                else // fixed annuity
                {
                    ann.wrn_msg += "fixing frequency provided for a fixed annuity;";
                    ann.fix_freq = "";
                }
            }

            // multiplier applied on internal repricing rate
            aux = rslt->tbl.values[ann_idx][21];
            if (aux.compare("") == 0) // rate multiplier not provided
            {
                ann.wrn_msg += "rate multiplier not provided for a floating annuity;";
                ann.rate_mult = 1.0;
            }
            else // rate multiplied provided
            {
                ann.rate_mult = stod(aux);
            }

            // spread to be added to a repricing rate
            aux = rslt->tbl.values[ann_idx][22];
            if (aux.compare("") == 0) // spread not specified
            {
                ann.wrn_msg += "repricing spread not provided for a flating annuity;";
            }
            else // spread specified
            {
                ann.rate_add = stod(aux);
            }

            // discounting curve
            ann.crv_disc = rslt->tbl.values[ann_idx][23];

            // repricing curve
            aux = rslt->tbl.values[ann_idx][24];
            if (aux.compare("") == 0) // repricing curve not specified
            {
                if (!ann.is_fixed) // floating annuity
                {
                    ann.wrn_msg += "repricing curve not provided for a floating annuity;";
                    ann.is_fixed = true;
                    ann.fix_freq = "";
                    ann.fix_type = "fix";
                }
                ann.crv_fwd = "";
            }
            else // repricing curve specified
            {
                if (!ann.is_fixed) // floating annuity
                {
                    ann.crv_fwd = aux;
                }
                else // fixed annuity
                {
                    ann.wrn_msg += "repricing curve specified for a fixed annuity;";
                    ann.crv_fwd = "";
                }
            }

        // perform other sanity checks
       
            // value date
            if (ann.value_date.get_days_no() > ann.maturity_date.get_days_no())
            {
                ann.wrn_msg += "value date cannot by greater than maturity date;";
                ann.value_date = ann.maturity_date;
            }

            // first annuity date
            if (ann.first_ann_date.get_days_no() <= ann.value_date.get_days_no())
            {
                ann.wrn_msg += "first annuity date cannot be lower or equal to value date;";
                ann.first_ann_date = ann.value_date;
                ann.first_ann_date.add(ann.ann_freq);
                while (ann.first_ann_date.get_days_no() < ann.value_date.get_days_no())
                {
                    ann.first_ann_date.add(ann.ann_freq);
                }
            }

            if (ann.first_ann_date.get_days_no() > ann.maturity_date.get_days_no())
            {
                ann.wrn_msg += "first annuity date cannot be greater than maturity date;";
                ann.first_ann_date = ann.maturity_date;
            }

            // first fixing date
            if (!ann.is_fixed)
            {
                if (ann.first_fix_date.get_days_no() <= ann.value_date.get_days_no())
                {
                    ann.wrn_msg += "first fixing date cannot be lower or equal to value date;";
                    ann.first_fix_date = ann.first_ann_date;
                }

                if (ann.first_fix_date.get_days_no() > ann.maturity_date.get_days_no())
                {
                    ann.wrn_msg += "first fixing date cannot be greater than maturity date;";
                    ann.first_fix_date = ann.first_ann_date;
                }

                if (ann.first_fix_date.get_days_no() <= ann.first_ann_date.get_days_no())
                {
                    ann.wrn_msg += "first fixing date cannot be lower or equal to the first annuity date;";
                    ann.first_fix_date = ann.first_ann_date;
                }
            }

        // check frequencies
        
            // repricing frequency
            if (ann.fix_freq.compare("") != 0)
            {
                if (eval_freq(ann.fix_freq) < eval_freq(ann.ann_freq))
                {
                    ann.wrn_msg += "repricing frequency cannot be lower than annuity frequency;";
                    ann.fix_freq = ann.ann_freq;
                }
            }

        // generate vector of events

            // variables containing dates
            myDate date1;
            myDate date2;

            // events
            ann_event evnt;
            std::vector<ann_event> events;

            // position index
            int pos_idx;

            // date format, which is used to store date string in object myDate
            std::string date_format = "yyyymmdd";

            // iterate until you find the first annuity period that does not preceed calculation date
            date1 = ann.first_ann_date;
            date2 = ann.first_ann_date;
            date2.remove(ann.ann_freq);
            while ((date1.get_days_no() < calc_date.get_days_no()) & (date2.get_days_no() < calc_date.get_days_no()))
            {
                date1 = date2;
                date2.add(ann.ann_freq);
            }
            
            // date1 represents the beginning date of such annuity period => we generate annuity dates till maturity
            event_dates = create_date_serie(date1.get_date_str(), ann.maturity_date.get_date_str(), ann.ann_freq, date_format);     

            // create a vector of events based on annuity dates
            for (int idx = 0; idx < event_dates.size(); idx++)
            {
                date1 = event_dates[idx];
                date2 = date1;
                date1.remove(ann.ann_freq);
                evnt.date_begin = date1; // beging of annuity period
                evnt.date_end = date2; // end of annuity period => date of annuity payment

                // number of remaining annuity payments
                evnt.rmng_ann_payments = event_dates.size() - idx;

                // indicated that annuity payment is fixed and therefore
                // could be calculated without a scenario knowledge
                if (ann.is_fixed)
                {
                    evnt.is_ann_fixed = true;
                }
                else
                {
                    // even a floating annuity has its first annuity payment
                    // fixed if it is not a forward contract
                    if (evnt.date_begin.get_days_no() < ann.first_fix_date.get_days_no())
                    {
                        evnt.is_ann_fixed = true;
                    }
                    // annuity payments in fugure
                    else
                    {
                        evnt.is_ann_fixed = false;
                    }
                }

                events.emplace_back(evnt);
            }

            // extract begin and end dates of the annuity periods
            begin_ann_dates = extract_dates_from_events(events, "date_begin");
            end_ann_dates = extract_dates_from_events(events, "date_end");

            // add repricing information
            if (ann.fix_freq.compare("") != 0)
            {
                // iterate until you find the first repricing date after calculation date
                date1 = ann.first_fix_date;
                while (date1.get_days_no() < calc_date.get_days_no())
                {
                    date1.add(ann.fix_freq);
                }

                // date1 is the first repricing date => we generate repricing dates till maturity
                event_dates = create_date_serie(date1.get_date_str(), ann.maturity_date.get_date_str(), ann.fix_freq, date_format);

                // determine in which coupon periods there will be repricing
                for (int idx = 0; idx < event_dates.size(); idx++)
                {
                    // we assign repricing flag based on begining coupon date
                    if (event_dates[idx].get_date_int() < ann.maturity_date.get_date_int())
                    {
                        pos_idx = get_nearest_idx(event_dates[idx], begin_ann_dates, "abs");
                        events[pos_idx].fix_flg = true;
                    }
                }

                // repricing dates
                if (ann.fix_type.compare("fix") != 0)
                {
                    for (int idx = 0; idx < events.size(); idx++)
                    {
                        // annuity rate was fixed in the previous annuity period; could occur if
                        // e.g annuity period is 6M and repricing period is 1Y
                        if (!events[idx].is_ann_fixed && !events[idx].fix_flg)
                        {
                            events[idx].int_rate = events[idx - 1].int_rate;
                            events[idx].ext_rate = events[idx - 1].ext_rate;
                        }
                        // determine annuity rate
                        else if (!events[idx].is_ann_fixed && events[idx].fix_flg)
                        {
                            date1 = events[idx].date_begin;
                            date2 = date1;
                            date2.add(ann.fix_freq);
                      
                            // all annuities are repriced with a par-rate; their
                            // amortization pattern depends on the annuity rate,
                            // so we can only collect date information
                            for (int idx2 = 0; idx2 < events.size(); idx2++)
                            {
                                // check that coupon date falls within
                                // the boundary repricing dates
                                if ((events[idx2].date_end.get_days_no() >= date1.get_days_no()) &&
                                    (events[idx2].date_end.get_days_no() <= date2.get_days_no()))
                                {
                                    events[idx].repricing_dates.emplace_back(events[idx2].date_end);
                                }
                            }
                        }
                    }
                }
            }

            // calculate annuity payment and nominal for fixed annuity payments
            for (int idx = 0; idx < events.size(); idx++)
            {
                // begining nominal is always known
                if (idx == 0)
                {
                    events[idx].nominal_begin = ann.nominal;
                }
                else
                {
                    events[idx].nominal_begin = events[idx - 1].nominal_end;
                }

                // determine annuity interest
                if (events[idx].date_begin.get_days_no() <= calc_date.get_days_no())
                {
                    events[idx].int_rate = ann.int_rate;
                    events[idx].ext_rate = ann.rate_mult * ann.int_rate + ann.rate_add;
                    events[idx].ext_ann_payment = calc_ann_payment(events[idx].ext_rate, events[idx].rmng_ann_payments, ann.ann_freq_aux) * events[idx].nominal_begin;
                }
                else if (!events[idx].fix_flg)
                {
                    events[idx].int_rate = events[idx - 1].int_rate;
                    events[idx].ext_rate = events[idx - 1].ext_rate;
                    events[idx].ext_ann_payment = events[idx - 1].ext_ann_payment;
                }

                // calculate interest, amortization and end nominal
                events[idx].ext_int_payment = events[idx].ext_rate / ann.ann_freq_aux * events[idx].nominal_begin;
                events[idx].int_int_payment = events[idx].int_rate / ann.ann_freq_aux * events[idx].nominal_begin;
                events[idx].ext_amort_payment = events[idx].ext_ann_payment - events[idx].ext_int_payment;
                events[idx].nominal_end = events[idx].nominal_begin - events[idx].ext_amort_payment;
                
                // calculate accrued interest if needed 
                if (events[idx].is_ann_fixed & !events[idx].fix_flg)
                {
                    // calculate accrued interest for the first coupon payment
                    // if neccessary
                    if (!ann.is_acc_int && (idx == 0))
                    {
                        if (events[idx].date_begin.get_days_no() <= calc_date.get_days_no())
                        {
                            // year fraction for the first coupon payment; we assume actual / 365 day count method
                            double acc_int_year_frac = day_count_method(events[idx].date_begin, calc_date, "ACT_365");
                            ann.ext_acc_int = events[idx].ext_rate * events[idx].nominal_begin * acc_int_year_frac;
                        }
                        // in case of a forward contract accrued interest is zero
                        else
                        {
                            ann.ext_acc_int = 0.0;
                        }
                    }

                    // check that the accured interest is not greater than interest
                    // part of the first annuity payment; please note we have to take into
                    // account also possibility of negative nominal
                    if (idx == 0)
                    {
                        // positive external rate
                        if (events[idx].ext_rate > 0.0)
                        {
                            // positive nominal
                            if (events[idx].nominal_begin > 0.0)
                            {
                                // accrued interest is greater than the
                                // first interest payment
                                if (ann.ext_acc_int > events[idx].ext_int_payment)
                                {
                                    ann.ext_acc_int = events[idx].ext_int_payment;
                                }
                            }
                            // negative nominal
                            else
                            {
                                // accrued interest is greater than the
                                // first interest payment
                                if (ann.ext_acc_int < events[idx].ext_int_payment)
                                {
                                    ann.ext_acc_int = events[idx].ext_int_payment;
                                }
                            }
                        }
                        // negative internal rate
                        else
                        {
                            // positive nominal
                            if (events[idx].nominal_begin > 0.0)
                            {
                                // accrued interest is greater than the
                                // first interest payment
                                if (ann.ext_acc_int < events[idx].ext_int_payment)
                                {
                                    ann.ext_acc_int = events[idx].ext_int_payment;
                                }
                            }
                            // negative nominal
                            else
                            {
                                // accrued interest is greater than the
                                // first interest payment
                                if (ann.ext_acc_int > events[idx].ext_int_payment)
                                {
                                    ann.ext_acc_int = events[idx].ext_int_payment;
                                }
                            }
                        }
                    }

                    // total cash-flow generated by the annuity
                    events[idx].ext_cf = events[idx].ext_amort_payment + events[idx].ext_int_payment;
                    events[idx].int_cf = events[idx].ext_amort_payment + events[idx].int_int_payment;
                }
            }
        
            // add events into bond structure
            ann.events = events;

            // add bond to vector of bonds
            this->info.emplace_back(ann);
        }

    // delete unused pointers
    delete rslt;
}

/*
 * OBJECT FUNCTIONS
 */

// split the object
std::vector<myAnnuities> myAnnuities::split(const int &threads_no)
{

    // determine position indicies of the newly created vectors
    std::vector<coordinates<int>> indicies = split_vector(this->info.size(), threads_no);

    // split original object myAnnuities into several smaller objects
    std::vector<myAnnuities> anns;
    for (int thread_idx = 0; thread_idx < threads_no; thread_idx++)
    {
        // determine the first and the last position index
        std::vector<ann_info>::const_iterator first = this->info.begin() + indicies[thread_idx].x;
        std::vector<ann_info>::const_iterator last = this->info.begin() + indicies[thread_idx].y + 1;

        // extract annuity information
        std::vector<ann_info> info(first, last);

        // create new smaller object based on the position indicies
        myAnnuities ann(info);

        // add the new object to the vector
        anns.emplace_back(ann);
    }

    // release memory by clearing the original object
    this->clear();

    // return vector of objects
    return anns;
}

// merge several objects
void myAnnuities::merge(std::vector<myAnnuities> &anns)
{

    // pre-allocate memory
    int final_vector_size = 0;
    std::vector<ann_info> info;
    
    for (int idx = 0; idx < anns.size(); idx++)
    {
        final_vector_size += anns[idx].info.size();
    }

    info.reserve(final_vector_size);

    // merge individual vectors
    for (int idx = 0; idx < anns.size(); idx++)
    {
        info.insert(info.end(), anns[idx].info.begin(), anns[idx].info.end());
        anns[idx].clear(); // release memory by clearing individual objects
    }

    // copy merged vectors into the current object
    this->info = info;

}

// calculate NPV
void myAnnuities::calc_npv(const int &scn_no, const myCurves &crvs, const myFx &fx, const std::string &ref_ccy_nm)
{
    // variables 
    std::vector<std::tuple<int, int>> scn_tenors;

    // go annuity by annuity
    for (int ann_idx = 0; ann_idx < this->info.size(); ann_idx++)
    {
        // calculate repricing rates for floating annuities
        if (!this->info[ann_idx].is_fixed)
        {
            // go event by event
            for (int idx = 0; idx < this->info[ann_idx].events.size(); idx++)
            {

                // update begining nominal
                if (!this->info[ann_idx].events[idx].is_ann_fixed)
                {
                    // update begin nominal
                    if (idx > 0)
                    {
                        this->info[ann_idx].events[idx].nominal_begin = this->info[ann_idx].events[idx - 1].nominal_end;
                    }
                }

                // rate is already fixed
                if (this->info[ann_idx].events[idx].is_ann_fixed | !this->info[ann_idx].events[idx].fix_flg)
                {
                    if (idx > 0)
                    {
                        this->info[ann_idx].events[idx].int_rate = this->info[ann_idx].events[idx - 1].int_rate;
                        this->info[ann_idx].events[idx].ext_rate = this->info[ann_idx].events[idx - 1].ext_rate;
                        this->info[ann_idx].events[idx].ext_ann_payment = this->info[ann_idx].events[idx - 1].ext_ann_payment;
                    }
                }
                // calculate repricing rate for unfixed events
                else
                {
                    // prepare vector with scenario number and tenors
                    scn_tenors.clear();
                    for (int idx2 = 0; idx2 < this->info[ann_idx].events[idx].repricing_dates.size(); idx2++)
                    {
                        scn_tenors.push_back(std::tuple<int, int>(scn_no, this->info[ann_idx].events[idx].repricing_dates[idx2].get_date_int()));
                    }

                    // extract vector of discounting factors
                    std::vector<double> dfs = crvs.get_df(this->info[ann_idx].crv_fwd, scn_tenors);

                    // estimate repricing annuity rate through Newton-Raphson method
                    int max_iter_no = 5;
                    double precission = 1e-10;
                    this->info[ann_idx].events[idx].int_rate = get_ann_rate(this->info[ann_idx].events[idx].rmng_ann_payments, this->info[ann_idx].ann_freq_aux, dfs, max_iter_no, precission);
                    this->info[ann_idx].events[idx].ext_rate = this->info[ann_idx].rate_mult * this->info[ann_idx].events[idx].int_rate + this->info[ann_idx].rate_add;
                    this->info[ann_idx].events[idx].ext_ann_payment = calc_ann_payment(this->info[ann_idx].events[idx].ext_rate, this->info[ann_idx].events[idx].rmng_ann_payments, this->info[ann_idx].ann_freq_aux) * this->info[ann_idx].events[idx].nominal_begin;
                }

                // update interest payments
                this->info[ann_idx].events[idx].ext_int_payment = this->info[ann_idx].events[idx].nominal_begin / this->info[ann_idx].ann_freq_aux * this->info[ann_idx].events[idx].ext_rate;
                this->info[ann_idx].events[idx].int_int_payment = this->info[ann_idx].events[idx].nominal_begin / this->info[ann_idx].ann_freq_aux * this->info[ann_idx].events[idx].int_rate;
                
                // calculate amortization
                this->info[ann_idx].events[idx].ext_amort_payment = this->info[ann_idx].events[idx].ext_ann_payment - this->info[ann_idx].events[idx].ext_int_payment;
                
                // update end nominal
                this->info[ann_idx].events[idx].nominal_end = this->info[ann_idx].events[idx].nominal_begin - this->info[ann_idx].events[idx].ext_amort_payment;
                
                // determine cash-flow
                this->info[ann_idx].events[idx].ext_cf = this->info[ann_idx].events[idx].ext_int_payment + this->info[ann_idx].events[idx].ext_amort_payment;
                this->info[ann_idx].events[idx].int_cf = this->info[ann_idx].events[idx].int_int_payment + this->info[ann_idx].events[idx].ext_amort_payment;

            }
        }

        // calculate discount factors and total bond NPV
        for (int idx = 0; idx < this->info[ann_idx].events.size(); idx++)
        {
            // prepare vector with scenario number and tenors
            std::vector<std::tuple<int, int>> scn_tenors;
            scn_tenors.push_back(std::tuple<int, int>(scn_no, this->info[ann_idx].events[idx].date_end.get_date_int()));
            
            // get discounting factor
            std::vector<double> dfs = crvs.get_df(this->info[ann_idx].crv_disc, scn_tenors);

            // store discount factor
            this->info[ann_idx].events[idx].df = dfs[0];

            // update NPV
            this->info[ann_idx].int_npv += this->info[ann_idx].events[idx].int_cf * this->info[ann_idx].events[idx].df;
            this->info[ann_idx].ext_npv += this->info[ann_idx].events[idx].ext_cf * this->info[ann_idx].events[idx].df;
        }

        // calculate NPV and accured interest in reference currency
        std::tuple<int, std::string> scn_ccy = std::tuple<int, std::string>(scn_no, ref_ccy_nm);
        double fx_rate = fx.get_fx(scn_ccy);

        this->info[ann_idx].ext_acc_int_ref_ccy = this->info[ann_idx].ext_acc_int * fx_rate;
        this->info[ann_idx].int_npv_ref_ccy = this->info[ann_idx].int_npv * fx_rate;
        this->info[ann_idx].ext_npv_ref_ccy = this->info[ann_idx].ext_npv * fx_rate;
    }
}

// calculate NPV using multithreading
std::thread myAnnuities::calc_npv_thrd(const int &scn_no, const myCurves &crvs, const myFx &fx, const std::string &ref_ccy_nm)
{
    std::thread worker(&myAnnuities::calc_npv, this, scn_no, crvs, fx, ref_ccy_nm);
    return worker;
}

// write NPV into SQLite database file
void myAnnuities::write_npv(mySQLite &db, const int &scn_no, const std::string &ent_nm, const std::string &ptf)
{
    // initiate dataframe structure
    dataFrame df;

    // add column names
    df.col_nms = {"scn_no", "ent_nm", "parent_id", "contract_id", "ptf", "ext_acc_int", "ext_npv", "int_npv", "ext_acc_int_ref_ccy", "ext_npv_ref_ccy", "int_npv_ref_ccy"};

    // add column data type
    df.dtypes = {"INT", "CHAR", "CHAR", "CHAR", "CHAR", "FLOAT", "FLOAT", "FLOAT", "FLOAT", "FLOAT", "FLOAT"};
    
    // add values
    std::vector<std::vector<std::string>> values;
    std::vector<std::string> ann;

    // go annuity by annuity
    for (int ann_idx = 0; ann_idx < this->info.size(); ann_idx++)
    {
        ann.clear();
        ann.push_back(std::to_string(scn_no));
        ann.push_back(this->info[ann_idx].ent_nm);
        ann.push_back(this->info[ann_idx].parent_id);
        ann.push_back(this->info[ann_idx].contract_id);
        ann.push_back(this->info[ann_idx].ptf);
        ann.push_back(std::to_string(this->info[ann_idx].ext_acc_int));
        ann.push_back(std::to_string(this->info[ann_idx].ext_npv));
        ann.push_back(std::to_string(this->info[ann_idx].int_npv));
        ann.push_back(std::to_string(this->info[ann_idx].ext_acc_int_ref_ccy));
        ann.push_back(std::to_string(this->info[ann_idx].ext_npv_ref_ccy));
        ann.push_back(std::to_string(this->info[ann_idx].int_npv_ref_ccy));
        values.push_back(ann);
    }
    df.values = values;

    // create dataframe object
    myDataFrame tbl = myDataFrame(df);

    // delete old data based on scenario number, entity name and portfolio
    std::string sql = "DELETE FROM ann_npv WHERE scn_no = " + std::to_string(scn_no) + " AND ent_nm = '" + ent_nm + "' AND ptf = '" + ptf + "';";
    db.exec(sql);

    // write dataframe into SQLite database file
    db.upload_tbl(tbl, "ann_npv", false);
}
