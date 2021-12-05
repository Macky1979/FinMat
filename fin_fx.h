#pragma once

#include <string>
#include <map>
#include <tuple>
#include "lib_sqlite.h"

/*
#include <string>
#include <iostream>
#include <vector>
#include <sqlite3.h>
#include "lib_dataframe.h"
#include "lib_date.h"
#include "lib_sqlite.h"
#include "lib_aux.h"
#include "fin_fx.h"

using namespace std;

int main()
{
    // variables
    const char * db_file_nm = "data/finmat.db";
    string sql_file_nm = "data/fx.sql";
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

    // create FX data table and index if it does not exists
    sql = read_sql(sql_file_nm, 1);
    db.exec(sql);
    sql = read_sql(sql_file_nm, 2);
    db.exec(sql);
    sql = "DELETE FROM fx_data;";
    db.exec(sql);

    // create dataframe from .csv file and store it into database
    rslt->read("data//fx_rates.csv", sep, quotes);
    db.upload_tbl(*rslt, "fx_data", delete_old_data);
    rslt->clear();

    // vacuum SQLite database file to avoid its excessive growth
    db.vacuum();

    // create curve object
    myFx fx = myFx(db, sql_file_nm);

    // close connection to SQLite database file
    db.close();

    // prepare tenors
    tuple<int, string> scn_ccy;
    int scn_no = 0;
    string ccy_nm = "CZK";
    scn_ccy = tuple<int, string>(scn_no, ccy_nm);

    // get FX rate
    float * fx_rate = fx.get_fx(scn_ccy); 
    cout << "FX rate for " + ccy_nm + ": " + to_string(*fx_rate) << endl;

    // delete unused pointer
    fx_rate = NULL;
    delete fx_rate;   

    // everything OK
    return 0;
}
*/


// define FX object
class myFx
{
    public:
        // object variables
        std::map<std::tuple<int, std::string>, float> data; // map based on scenario number and currency name

        // object constructors
        myFx(const mySQLite &db, const std::string &sql_file_nm);

        // object destructor
        ~myFx(){};

        // object function declarations
        float * get_fx(const std::tuple<int, std::string> &ccy);
};
