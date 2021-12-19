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
#include <memory>
#include "lib_dataframe.h"
#include "lib_sqlite.h"
#include "fin_fx.h"

int main()
{
    // variables
    const char * db_file_nm = "data/finmat.db";
    std::string sql_file_nm = "data/finmat.sql";
    std::string cnty_def = "data/cnty_def.csv";
    std::string ccy_def = "data/ccy_def.csv";
    std::string ccy_data = "data/ccy_data.csv";
    std::string sql;
    myDataFrame * rslt = new myDataFrame();
    std::string sep = ",";
    bool quotes = false;
    bool read_only;
    int wait_max_seconds = 10;
    bool delete_old_data = false;

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

    // delete old content in the tables
    db.exec("DELETE FROM cnty_def;");
    db.exec("DELETE FROM ccy_def;");
    db.exec("DELETE FROM ccy_data;");

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

    // vacuum SQLite database file to avoid its excessive growth
    db.vacuum();

    // create object with FX rates
    myFx fx = myFx(db, sql_file_nm);

    // close connection to SQLite database file
    db.close();

    // print CZK / EUR rate
    std::tuple<int, std::string> scn_ccy = {1, "CZK"};
    float * fx_rate;
    fx_rate = fx.get_fx(scn_ccy);
    std::cout << "CZK / EUR rate for scenario 1: " + std::to_string(*fx_rate) << std::endl;

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
