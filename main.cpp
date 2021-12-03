#include <string>
#include <iostream>
#include <vector>
#include <sqlite3.h>
#include "lib_dataframe.h"
#include "lib_date.h"
#include "lib_sqlite.h"
#include "lib_aux.h"
#include "fin_curve.h"

using namespace std;

int main()
{
    // variables
    myDate calc_date = myDate(20121202);
    const char * db_file_nm = "data/finmat.db";
    string sql_file_nm = "data/curves.sql";
    string sql;
    myDataFrame * rslt = new myDataFrame();
    bool read_only;
    int wait_max_seconds = 15;
    bool delete_old_data = false;
    string sep = ",";
    bool quotes = false;

    // create SQLite object and open connection to SQLite database file in read-write mode
    read_only = false;
    mySQLite db(db_file_nm, read_only, wait_max_seconds);

    // create curve definition table and index if it does not exists
    sql = read_sql(sql_file_nm, 1);
    db.exec(sql);
    sql = read_sql(sql_file_nm, 2);
    db.exec(sql);
    sql = "DELETE FROM crv_def;";
    db.exec(sql);

    // create curve data table and index if it does not exists
    sql = read_sql(sql_file_nm, 3);
    db.exec(sql);
    sql = read_sql(sql_file_nm, 4);
    db.exec(sql);
    sql = "DELETE FROM crv_data;";
    db.exec(sql);

    // create dataframes from .csv files and store them into database
    rslt->read("data//curve_definitions.csv", sep, quotes);
    db.upload_tbl(*rslt, "crv_def", delete_old_data);
    rslt->clear();

    rslt->read("data//interbcrv_eur.csv", sep, quotes);
    db.upload_tbl(*rslt, "crv_data", delete_old_data);
    rslt->clear();

    rslt->read("data//spreadcrv_bef.csv", sep, quotes);
    db.upload_tbl(*rslt, "crv_data", delete_old_data);
    rslt->clear();

    // vacuum SQLite database file to avoid its excessive growth
    db.vacuum();

    // create curve object
    myCurve crv = myCurve(db, sql_file_nm, "INTERBCRV.EUR", calc_date);

    // close connection to SQLite database file
    db.close();

    // prepare tenors
    vector<tuple<int, string>> scn_tenors;
    vector <string> maturities = {"20130602", "20131202", "20140604", "20141202", "20150603"};
    int scn_no = 0;
    for (int i = 0; i < maturities.size(); i++)
    {
        scn_tenors.push_back(tuple<int, string>(scn_no, maturities[i]));
    }

    // get zero rates
    vector<float> * zeros = new vector<float>();
    zeros = crv.get_zero_rate(scn_tenors);

    for (int i = 0; i < maturities.size(); i++)
    {
        cout << "zero rate for " + maturities[i] + ": " + to_string((*zeros)[i]) << endl;
    }

    delete zeros;   

    // get discount factors
    vector<float> * dfs = new vector<float>();
    dfs = crv.get_df(scn_tenors);

    for (int i = 0; i < maturities.size(); i++)
    {
        cout << "discount factor for " + maturities[i] + ": " + to_string((*dfs)[i]) << endl;
    }

    delete dfs;

    // get forward rates
    vector<float> * fwds = new vector<float>();
    fwds = crv.get_fwd_rate(scn_tenors, "ACT_365");

    for (int i = 0; i < maturities.size() - 1; i++)
    {
        cout << "forward rate for " + maturities[i] + " - " + maturities[i + 1] + ": " + to_string((*fwds)[i]) << endl;
    }

    delete fwds;

    // get par rates
    int step = 3;
    vector<float> * pars = new vector<float>();
    vector <float> nominals = {100., 100., 100., 100., 100.};
    pars = crv.get_par_rate(scn_tenors, nominals, step, "ACT_365");

    for (int i = 0; i < maturities.size() - step; i++)
    {
        cout << "par rate for " + maturities[i] + " - " + maturities[i + step] + ": " + to_string((*pars)[i]) << endl;
    }

    delete pars;

    // everything OK
    return 0;
}
