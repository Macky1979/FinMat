#include <string>
#include <map>
#include <math.h>
#include <tuple> 
#include <vector>
#include "lib_sqlite.h"
#include "lib_lininterp.h"
#include "fin_date.h"
#include "fin_curve.h"

/*
 * OBJECT CONSTRUCTORS
 */

// object containing information on a single curve
myCurve::myCurve(const mySQLite &db, const std::string &sql_file_nm, const std::string &crv_nm, const myDate &calc_date)
{
    // dataframe to hold result of SQL query
    myDataFrame * rslt = new myDataFrame();

    // variable to hold SQL query
    std::string sql;

    // load curve definition
    sql = read_sql(sql_file_nm, "load_crv_def");
    sql = replace_in_sql(sql, "##crv_nm##", "'" + crv_nm + "'");
    rslt = db.query(sql);

    // initiate curve definition variables
    this->calc_date = calc_date;
    this->crv_nm = rslt->tbl.values[0][0];
    this->ccy_nm = rslt->tbl.values[0][1];
    this->dcm = rslt->tbl.values[0][2];
    this->crv_type = rslt->tbl.values[0][3];
    this->underlying1 = rslt->tbl.values[0][4];
    this->underlying2 = rslt->tbl.values[0][5];

    // base curve
    if (this->crv_type.compare("base") == 0)
    {
        // retrieve data
        sql = read_sql(sql_file_nm, "load_base_crv_data");
        sql = replace_in_sql(sql, "##crv_nm##", "'" + this->crv_nm + "'");
        rslt = db.query(sql);
    
    }
    // compound curve
    else if (this->crv_type.compare("compound") == 0)
    {
        // retrieve data
        sql = read_sql(sql_file_nm, "load_compound_crv_data");
        sql = replace_in_sql(sql, "##crv_nm1##", "'" + this->underlying1 + "'");
        sql = replace_in_sql(sql, "##crv_nm2##", "'" + this->underlying2 + "'");
        rslt = db.query(sql);
    }
    // unsupported curve type
    else
    {
        throw std::invalid_argument((std::string)__func__ + ": " + this->crv_type + " is not a supported date std::string format!");
    }

    // prepare vector of tenors for which we want to interporate the curve
    std::vector<double> tenors;
    std::vector<myDate> tenor_dates;
    for (double tenor = 1; tenor < 120 * 365 + 1; tenor++)
    {
        tenors.push_back(tenor);
        myDate tenor_date = this->calc_date;
        tenor_date.add(std::to_string(tenor) + "D");
        tenor_dates.push_back(tenor_date);
    }

    // go scenario by scenario
    int scn_no = -1;
    bool is_new_scn = false;
    std::vector<double> _tenors;
    std::vector<double> _rates;

    for (int idx = 0; idx < rslt->tbl.values.size(); idx++)
    {
        // check that you have encountered a new scenario or you are at the end of file
        if (((stoi(rslt->tbl.values[idx][0]) != scn_no) && (scn_no != -1)) || (idx == rslt->tbl.values.size() - 1))
        {
            // interpolate rates
            myLinInterp interp(_tenors, _rates);
            std::vector<double> rates = interp.eval(tenors);

            // go scenario line by line
            for (int idx2 = 0; idx2 < rates.size(); idx2++)
            {
                // create a tuple of scenario and tenor date integer in yyyymmdd format
                std::tuple<int, int> scn_tenor = {scn_no, tenor_dates[idx2].get_date_int()};

                // create tenor and store rate into it
                tenor_def tenor;
                tenor.tenor = tenors[idx2];
                tenor.tenor_date = tenor_dates[idx2];
                tenor.rate = rates[idx2];

                // calculate year fraction
                tenor.year_frac = day_count_method(this->calc_date, tenor.tenor_date, this->dcm);
                tenor.year_frac_aux2 = floor(tenor.year_frac);
                tenor.year_frac_aux1 = tenor.year_frac - tenor.year_frac_aux2;

                // calculate discount factor
                tenor.df = 1. / (1 + tenor.rate * tenor.year_frac_aux1) * 1. / pow((1 + tenor.rate), tenor.year_frac_aux2);

                // calculate zero rate
                tenor.zero_rate = pow(tenor.df, -1. / tenor.year_frac) - 1;

                // add tenor to curve object
                this->tenor.insert(std::pair<std::tuple<int, int>, tenor_def>(scn_tenor, tenor));

                // update scenario and clear variables
                _tenors.clear();
                _rates.clear();
            }
        }
        // load data
        {
            scn_no = stoi(rslt->tbl.values[idx][0]);
            _tenors.push_back(stod(rslt->tbl.values[idx][1]));
            _rates.push_back(stod(rslt->tbl.values[idx][2]));
        }
    }

    // delete unused pointers
    delete rslt;
}

// object containing information on all curves
myCurves::myCurves(const mySQLite &db, const std::string &sql_file_nm, const myDate &calc_date)
{
    // dataframe to hold result of SQL query
    myDataFrame * rslt = new myDataFrame();

    // variable to hold SQL query
    std::string sql;

    // load list of curves
    sql = read_sql(sql_file_nm, "load_all_crv_nms");
    rslt = db.query(sql);

    // load curve by curve
    std::string crv_nm;
    for (int crv_idx = 0; crv_idx < rslt->tbl.values.size(); crv_idx++)
    {
        crv_nm = rslt->tbl.values[crv_idx][0];
        myCurve crv = myCurve(db, sql_file_nm, crv_nm, calc_date);
        this->crv.insert(std::pair<std::string, myCurve>(crv_nm, crv));
    }

    // delete unused pointers
    delete rslt;
}

/*
 * OBJECT FUNCTIONS
 */

// get year fraction based on vector of scenario numbers and tenor integer dates in yyyymmdd format
std::vector<double> myCurve::get_year_frac(const std::vector<std::tuple<int, int>> &tenor) const
{
    // create vector to hold data
    std::vector<double> year_fracs;

    // go through the tenors on input
    for (int idx = 0; idx < tenor.size(); idx++)
    {
        year_fracs.push_back(this->tenor.at(tenor[idx]).year_frac);
    }

    // return vector of zero rates
    return year_fracs;
}

// get tenor dates based on vector of scenario numbers and tenor integer dates in yyyymmdd format
std::vector<myDate> myCurve::get_tenor_dates(const std::vector<std::tuple<int, int>> &tenor) const
{
    // create vector to hold data
    std::vector<myDate> tenor_dates;

    // go through the tenors on input
    for (int idx = 0; idx < tenor.size(); idx++)
    {
        tenor_dates.push_back(this->tenor.at(tenor[idx]).tenor_date);
    }

    // return vector of zero rates
    return tenor_dates;
}

// get zero rate based on vector of scenario numbers and tenor std::string dates in yyyymmdd format
std::vector<double> myCurve::get_zero_rate(const std::vector<std::tuple<int, int>> &tenor) const
{
    // create vector to hold data
    std::vector<double> zero_rates;

    // go through the tenors on input
    for (int idx = 0; idx < tenor.size(); idx++)
    {
        zero_rates.push_back(this->tenor.at(tenor[idx]).zero_rate);
    }

    // return vector of zero rates
    return zero_rates;
}

// get discount factor based on vector of scenario numbers and tenors
std::vector<double> myCurve::get_df(const std::vector<std::tuple<int, int>> &tenor) const
{
    // create vector to hold data
    std::vector<double> dfs;

    // go through the tenors on input
    for (int idx = 0; idx < tenor.size(); idx++)
    {
        dfs.push_back(this->tenor.at(tenor[idx]).df);
    }

    // return vector of zero rates
    return dfs;
}

// calculate forward rate based on vector scenarios numbers and tenors
std::vector<double> myCurve::get_fwd_rate(const std::vector<std::tuple<int, int>> &tenor, const std::string &dcm) const
{
    // create vector to hold data
    std::vector<double> fwds;

    // get tenor dates and corresponding discount factors
    std::vector<myDate> tenor_dates = this->get_tenor_dates(tenor);
    std::vector<double> dfs = this->get_df(tenor);
    
    // calculate forward rates
    for (int idx = 0; idx < dfs.size() - 1; idx++)
    {
        double df1 = dfs[idx];
        double df2 = dfs[idx + 1];
        double d_t = day_count_method(tenor_dates[idx], tenor_dates[idx + 1], dcm);
        double fwd = (df1 / df2 - 1) / d_t;
        fwds.push_back(fwd);
    }

    // return vector of forward rates
    return fwds;
}

// calculate par rate based on vector scenarios numbers, tenors and nominals
std::vector<double> myCurve::get_par_rate(const std::vector<std::tuple<int, int>> &tenor, const std::vector<double> &nominals_begin, const std::vector<double> &nominals_end, const int &step, const std::string &dcm) const
{
    // create vector to hold data
    std::vector<double> pars;

    // get discount factors and year fractions
    std::vector<myDate> tenor_dates = this->get_tenor_dates(tenor);
    std::vector<double> dfs = this->get_df(tenor);

    // calculate par-rate
    double par;
    for (int idx = 0; idx < dfs.size() - step; idx++)
    {
        // nominals
        par = dfs[idx] * nominals_end[idx] - dfs[idx + step] * nominals_end[idx + step];

        double aux = 0;
        for (int idx2 = 0; idx2 < step; idx2++)
        {
            // amortizaton payments
            double amort = nominals_begin[idx + idx2 + 1] - nominals_end[idx + idx2 + 1];
            par -= dfs[idx + idx2 + 1] * amort;

            // coupon payments
            double d_t = day_count_method(tenor_dates[idx + idx2], tenor_dates[idx + idx2 + 1], dcm);
            aux += d_t * dfs[idx + idx2 + 1] * nominals_begin[idx + idx2 + 1];            
        }

        // calculate par-rate
        par /= aux;

        // store par-rate
        pars.push_back(par);
    }
    
    // return vector of forward rates
    return pars;
}

// get year fraction based on vector of scenario numbers and tenor integer dates in yyyymmdd format
std::vector<double> myCurves::get_year_frac(const std::string &crv_nm, const std::vector<std::tuple<int, int>> &tenor) const
{
    return this->crv.at(crv_nm).get_year_frac(tenor);
}

// get tenor dates based on vector of scenario numbers and tenor integer dates in yyyymmdd format
std::vector<myDate> myCurves::get_tenor_dates(const std::string &crv_nm, const std::vector<std::tuple<int, int>> &tenor) const
{
    return this->crv.at(crv_nm).get_tenor_dates(tenor);
}

// get zero rate based on vector of scenario numbers and tenor std::string dates in yyyymmdd format
std::vector<double> myCurves::get_zero_rate(const std::string &crv_nm, const std::vector<std::tuple<int, int>> &tenor) const
{
    return this->crv.at(crv_nm).get_zero_rate(tenor);
}

// get discount factor based on vector of scenario numbers and tenors
std::vector<double> myCurves::get_df(const std::string &crv_nm, const std::vector<std::tuple<int, int>> &tenor) const
{
    return this->crv.at(crv_nm).get_df(tenor);
}

// calculate forward rate based on vector scenarios numbers and tenors
std::vector<double> myCurves::get_fwd_rate(const std::string &crv_nm, const std::vector<std::tuple<int, int>> &tenor, const std::string &dcm) const
{
    return this->crv.at(crv_nm).get_fwd_rate(tenor, dcm); 
}

// calculate par rate based on vector scenarios numbers, tenors and nominals
std::vector<double> myCurves::get_par_rate(const std::string &crv_nm, const std::vector<std::tuple<int, int>> &tenor, const std::vector<double> &nominals_begin, const std::vector<double> &nominals_end, const int &step, const std::string &dcm) const
{
    return this->crv.at(crv_nm).get_par_rate(tenor, nominals_begin, nominals_end, step, dcm); 
}
