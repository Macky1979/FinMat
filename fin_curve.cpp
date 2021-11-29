#include <string>
#include <map>
#include <math.h> 
#include <vector>
#include "lib_sqlite.h"
#include "lib_lininterp.h"
#include "fin_curve.h"

using namespace std;

/*
 * OBJECT CONSTRUCTORS
 */

myCurve::myCurve(const mySQLite &db, string sql_file_nm, string crv_nm)
{
    // dataframe to hold result of SQL query
    myDataFrame * rslt = new myDataFrame();

    // variable to holds SQL query
    string sql;

    // load curve definition
    sql = read_sql(sql_file_nm, 5);
    sql = replace_in_sql(sql, "##crv_nm##", "'" + crv_nm + "'");
    rslt = db.query(sql);

    // initiate curve definition variables
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
        sql = read_sql(sql_file_nm, 6);
        sql = replace_in_sql(sql, "##crv_nm##", "'" + this->underlying1 + "'");
        rslt = db.query(sql);
    
    }
    // compound curve
    else if (this->crv_type.compare("compound") == 0)
    {
        // retrieve data
        sql = read_sql(sql_file_nm, 7);
        sql = replace_in_sql(sql, "##crv_nm1##", "'" + this->underlying1 + "'");
        sql = replace_in_sql(sql, "##crv_nm2##", "'" + this->underlying2 + "'");
        rslt = db.query(sql);
    }
    // unsupported curve type
    else
    {
        throw invalid_argument((string)__func__ + ": " + this->crv_type + " is not a supported date string format!");
    }

    // prepare vector of tenors for which we want to interporate the curve
    vector<float> tenors;
    for (float tenor = 1; tenor < 120 * 365 + 1; tenor++)
    {
        tenors.push_back(tenor);
    }

    // go scenario by scenario
    int scn_no = -1;
    bool is_new_scn = false;
    vector<float> _tenors;
    vector<float> _rates;


    for (int idx = 0; idx < rslt->tbl.values.size(); idx++)
    {
        // check that you have encountered a new scenario or you are at the end of file
        if (((stoi(rslt->tbl.values[idx][0]) != scn_no) && (scn_no != -1)) || idx == rslt->tbl.values.size() - 1)
        {
            // interpolate rates
            myLinInterp interp(_tenors, _rates);
            vector<float> * rates = new vector<float>();
            rates = interp.eval(tenors);

            // go scenario line by line
            for (int idx2 = 0; idx2 < rates->size(); idx2++)
            {
                // create a tuple of scenario and tenor
                tuple<int, int> scn_tenor = {scn_no, int(tenors[idx2])};

                // create tenor and store rate into it
                tenor_def tenor;
                tenor.rate = (*rates)[idx2];

                // calculate year fraction
                if (this->dcm.compare("ACT_365") == 0)
                {
                    tenor.year_frac =  tenors[idx2] / 365.;
                }
                else if (this->dcm.compare("ACT_360") == 0)
                {
                    tenor.year_frac =  tenors[idx2] / 360.;
                }
                else
                {
                    throw invalid_argument((string)__func__ + ": " + this->dcm + " is not a supported date count method!");
                }

                // calculate discount factor
                tenor.year_frac_aux2 = floor(tenor.year_frac);
                tenor.year_frac_aux1 = tenor.year_frac - tenor.year_frac_aux2;
                tenor.df = 1. / (1 + tenor.rate * tenor.year_frac_aux1) * 1. / pow((1 + tenor.rate), tenor.year_frac_aux2);

                // calculate zero rate
                tenor.zero_rate = pow(tenor.df, -1. / tenor.year_frac) - 1;

                // add tenor to curve object
                this->tenor.insert(pair<tuple<int, int>, tenor_def>(scn_tenor, tenor));

                // update scenario and clear variables
                scn_no = stoi(rslt->tbl.values[idx][0]);
                _tenors.clear();
                _rates.clear();
            }
        }
        // load data
        {
            _tenors.push_back(stod(rslt->tbl.values[idx][1]));
            _rates.push_back(stod(rslt->tbl.values[idx][2]));
        }
    }
}

/*
 * OBJECT FUNCTIONS
 */

// get year fraction based on vector of scenario numbers and tenors
vector<float> * myCurve::get_year_frac(const vector<tuple<int, int>> &tenor)
{
    // create vector to hold data
    vector<float> * year_fracs = new vector<float>();

    // go through the tenors on input
    for (int idx = 0; idx < tenor.size(); idx++)
    {
        year_fracs->push_back(this->tenor.at(tenor[idx]).year_frac);
    }

    // return pointer to vector of zero rates
    return year_fracs;
}

// get zero rate based on vector of scenario numbers and tenors
vector<float> * myCurve::get_zero_rate(const vector<tuple<int, int>> &tenor)
{
    // create vector to hold data
    vector<float> * zero_rates = new vector<float>();

    // go through the tenors on input
    for (int idx = 0; idx < tenor.size(); idx++)
    {
        zero_rates->push_back(this->tenor.at(tenor[idx]).zero_rate);
    }

    // return pointer to vector of zero rates
    return zero_rates;
}

// get discount factor based on vector of scenario numbers and tenors
vector<float> * myCurve::get_df(const vector<tuple<int, int>> &tenor)
{
    // create vector to hold data
    vector<float> * dfs = new vector<float>();

    // go through the tenors on input
    for (int idx = 0; idx < tenor.size(); idx++)
    {
        dfs->push_back(this->tenor.at(tenor[idx]).df);
    }

    // return pointer to vector of zero rates
    return dfs;
}

// calculate forward rate based on vector scenarios numbers and tenors
vector<float> * myCurve::get_fwd_rate(const vector<tuple<int, int>> &tenor)
{
    // create vectors to hold data
    vector<float> * dfs = new vector<float>();
    vector<float> * year_fracs = new vector<float>();
    vector<float> * fwds = new vector<float>();

    // get discount factors and year fractions
    dfs = this->get_df(tenor);
    year_fracs = this->get_year_frac(tenor);

    // calculate forward rates
    for (int idx = 0; idx < dfs->size() - 1; idx++)
    {
        float df1 = (*dfs)[idx];
        float df2 = (*dfs)[idx + 1];
        float d_t = (*year_fracs)[idx + 1] - (*year_fracs)[idx];
        float fwd = (df1 / df2 - 1) / d_t;
        fwds->push_back(fwd);
    }

    // delete unsed pointers
    delete dfs;
    delete year_fracs;

    // return pointer to vector of forward rates
    return fwds;
}

// calculate par rate based on vector scenarios numbers, tenors and nominals
std::vector<float> * myCurve::get_par_rate(const vector<tuple<int, int>> &tenor, const vector<float> &nominals, const int &step)
{
    // create vectors to hold data
    vector<float> * dfs = new vector<float>();
    vector<float> * year_fracs = new vector<float>();
    vector<float> * pars = new vector<float>();

    // get discount factors and year fractions
    year_fracs = this->get_year_frac(tenor);
    dfs = this->get_df(tenor);

    // calculate par rates
    for (int idx = 0; idx < dfs->size() - step; idx++)
    {
        float par = (*dfs)[idx] * nominals[idx] - (*dfs)[idx + step] * nominals[idx + step];
        float aux = 0;
        for (int idx2 = 0; idx2 < step; idx2++)
        {
            float d_t = (*year_fracs)[idx + idx2 + 1] - (*year_fracs)[idx + idx2];
            aux += d_t * (*dfs)[idx + idx2] * nominals[idx + idx2];;
        }
        par /= aux;
        pars->push_back(par);
    }

    // delete unsed pointers
    delete dfs;
    delete year_fracs;

    // return pointer to vector of forward rates
    return pars;
}