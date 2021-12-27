# pragma once

#include <string>
#include <map>
#include <tuple>
#include "lib_sqlite.h"
#include "lib_date.h"

/*
#include <string>
#include <iostream>
#include <vector>
#include <sqlite3.h>
#include <memory>
#include "lib_dataframe.h"
#include "lib_sqlite.h"
#include "fin_fx.h"
#include "fin_bond.h"

int main()
{
    // variables
    const char * db_file_nm = "data/finmat.db";
    std::string sql_file_nm = "data/finmat.sql";
    std::string cnty_def = "data/cnty_def.csv";
    std::string ccy_def = "data/ccy_def.csv";
    std::string ccy_data = "data/ccy_data.csv";
    std::string freq_def = "data/freq_def.csv";
    std::string dcm_def = "data/dcm_def.csv";
    std::string crv_def = "data/crv_def.csv";
    std::string interbcrv_eur = "data/curves/interbcrv_eur.csv";
    std::string spreadcrv_bef = "data/curves/spreadcrv_bef.csv";
    std::string sql;
    myDataFrame * rslt = new myDataFrame();
    myDate calc_date = myDate(20211203);
    std::string sep = ",";
    bool quotes = false;
    bool read_only;
    int wait_max_seconds = 10;
    bool delete_old_data = false;
    std::string crv_nm = "INTERBCRV.EUR";
    int scn_no = 1;

    // create SQLite object and open connection to SQLite database file in read-write mode
    read_only = false;
    mySQLite db(db_file_nm, read_only, wait_max_seconds);

    // create tables in SQLite database file if they do not exist
    sql = read_sql(sql_file_nm, "cnty_def");
    db.exec(sql);

    sql = read_sql(sql_file_nm, "ccy_def");
    db.exec(sql);

    sql = read_sql(sql_file_nm, "ccy_data");
    db.exec(sql);

    sql = read_sql(sql_file_nm, "freq_def");
    db.exec(sql);

    sql = read_sql(sql_file_nm, "dcm_def");
    db.exec(sql);

    sql = read_sql(sql_file_nm, "crv_def");
    db.exec(sql);

    sql = read_sql(sql_file_nm, "crv_data");
    db.exec(sql);

    // delete old content in the tables
    db.exec("DELETE FROM cnty_def;");
    db.exec("DELETE FROM ccy_def;");
    db.exec("DELETE FROM ccy_data;");
    db.exec("DELETE FROM freq_def;");
    db.exec("DELETE FROM dcm_def;");
    db.exec("DELETE FROM crv_def;");
    db.exec("DELETE FROM crv_data;");

    // create dataframes from .csv files and store them into database
    rslt->read(cnty_def, sep, quotes);
    db.upload_tbl(*rslt, "cnty_def", delete_old_data);
    rslt->clear();

    rslt->read(ccy_def, sep, quotes);
    db.upload_tbl(*rslt, "ccy_def", delete_old_data);
    rslt->clear();
    
    rslt->read(ccy_data, sep, quotes);
    db.upload_tbl(*rslt, "ccy_data", delete_old_data);
    rslt->clear();

    rslt->read(freq_def, sep, quotes);
    db.upload_tbl(*rslt, "freq_def", delete_old_data);
    rslt->clear();
    
    rslt->read(dcm_def, sep, quotes);
    db.upload_tbl(*rslt, "dcm_def", delete_old_data);
    rslt->clear();

    rslt->read(crv_def, sep, quotes);
    db.upload_tbl(*rslt, "crv_def", delete_old_data);
    rslt->clear();

    rslt->read(interbcrv_eur, sep, quotes);
    db.upload_tbl(*rslt, "crv_data", delete_old_data);
    rslt->clear();
   
    rslt->read(spreadcrv_bef, sep, quotes);
    db.upload_tbl(*rslt, "crv_data", delete_old_data);
    rslt->clear();

    // vacuum SQLite database file to avoid its excessive growth
    db.vacuum();

    // load all curves
    myCurves crvs = myCurves(db, sql_file_nm, calc_date);

    // close connection to SQLite database file
    db.close();

    // prepare tenors
    std::vector<std::tuple<int, int>> scn_tenors;
    std::vector <int> maturities = {20220603, 20221203, 20230605, 20231203, 20240603};
    for (int i = 0; i < maturities.size(); i++)
    {
        scn_tenors.push_back(std::tuple<int, int>(scn_no, maturities[i]));
    }

    // get zero rates
    std::vector<double> zeros = crvs.get_zero_rate(crv_nm, scn_tenors);

    for (int i = 0; i < maturities.size(); i++)
    {
        std::cout << "zero rate for " + std::to_string(maturities[i]) + ": " + std::to_string(zeros[i]) << std::endl;
    }

    // get discount factors
    std::vector<double> dfs = crvs.get_df(crv_nm, scn_tenors);

    for (int i = 0; i < maturities.size(); i++)
    {
        std::cout << "discount factor for " + std::to_string(maturities[i]) + ": " + std::to_string(dfs[i]) << std::endl;
    }

    // get forward rates
    std::vector<double> fwds = crvs.get_fwd_rate(crv_nm, scn_tenors, "ACT_365");

    for (int i = 0; i < maturities.size() - 1; i++)
    {
        std::cout << "forward rate for " + std::to_string(maturities[i]) + " - " + std::to_string(maturities[i + 1]) + ": " + std::to_string(fwds[i]) << std::endl;
    }

    // get par rates
    int step = 3;
    std::vector <double> begin_nominals = {100., 100., 100., 100., 100.};
    std::vector <double> end_nominals = {100., 100., 100., 100., 100.};
    std::vector<double> pars = crvs.get_par_rate(crv_nm, scn_tenors, begin_nominals, end_nominals, step, "ACT_365");

    for (int i = 0; i < maturities.size() - step; i++)
    {
        std::cout << "par rate for " + std::to_string(maturities[i]) + " - " + std::to_string(maturities[i + step]) + ": " + std::to_string(pars[i]) << std::endl;
    }

    // everything OK
    return 0;
}
*/

// tenor structure
struct tenor_def
{
    myDate tenor_date;
    int tenor;
    double rate;
    double year_frac;
    double year_frac_aux1;
    double year_frac_aux2;
    double df;
    double zero_rate;
};

// define curve class
class myCurve
{
    public:
        // object variables
        myDate calc_date;
        std::string crv_nm;
        std::string ccy_nm;
        std::string dcm;
        std::string crv_type;
        std::string underlying1;
        std::string underlying2;
    
        std::map<std::tuple<int, int>, tenor_def> tenor; // map based on scenario number and tenor date integer in yyyymmdd format
    
        // object constructors
        myCurve(const mySQLite &db, const std::string &sql_file_nm, const std::string &crv_nm, const myDate &calc_date);

        // object destructors
        ~myCurve(){};

        // object function declarations
        std::vector<double> get_year_frac(const std::vector<std::tuple<int, int>> &tenor) const;
        std::vector<myDate> get_tenor_dates(const std::vector<std::tuple<int, int>> &tenor) const;
        std::vector<double> get_zero_rate(const std::vector<std::tuple<int, int>> &tenor) const;
        std::vector<double> get_df(const std::vector<std::tuple<int, int>> &tenor) const;
        std::vector<double> get_fwd_rate(const std::vector<std::tuple<int, int>> &tenor, const std::string &dcm) const;
        std::vector<double> get_par_rate(const std::vector<std::tuple<int, int>> &tenor, const std::vector<double> &nominals, const std::vector<double> &amorts, const int &step, const std::string &dcm) const;
};

// define curves class
class myCurves
{
    public:
        // object variables
        std::map<std::string, myCurve> crv; // map based on curve name

        // object constructors
        myCurves(const mySQLite &db, const std::string &sql_file_nm, const myDate &calc_date);

        // object destructors
        ~myCurves(){};

        // object function declarations
        std::vector<double> get_year_frac(const std::string &crv_nm, const std::vector<std::tuple<int, int>> &tenor) const;
        std::vector<myDate> get_tenor_dates(const std::string &crv_nm, const std::vector<std::tuple<int, int>> &tenor) const;
        std::vector<double> get_zero_rate(const std::string &crv_nm, const std::vector<std::tuple<int, int>> &tenor) const;
        std::vector<double> get_df(const std::string &crv_nm, const std::vector<std::tuple<int, int>> &tenor) const;
        std::vector<double> get_fwd_rate(const std::string &crv_nm, const std::vector<std::tuple<int, int>> &tenor, const std::string &dcm) const;
        std::vector<double> get_par_rate(const std::string &crv_nm, const std::vector<std::tuple<int, int>> &tenor, const std::vector<double> &nominals, const std::vector<double> &amorts, const int &step, const std::string &dcm) const;
};
