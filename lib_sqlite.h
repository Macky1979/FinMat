#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <sqlite3.h>
#include "lib_dataframe.h"

/*
    // variables
    const char * db_file_nm = "database.db";
    string sql = "SELECT * FROM cities;";
    myDataFrame * rslt = new myDataFrame();

    // create SQLite object and open connection to SQLite database file in read-write mode
    mySQLite db(db_file_nm, false);

    // create table if it does not exists
    db.exec("CREATE TABLE IF NOT EXISTS cities (city VARCHAR(20), country VARCHAR(20));");

    // delete table
    db.exec("DELETE FROM cities;");   

    // insert data into table
    db.exec("INSERT INTO cities (city, country) VALUES ('Prague', 'Czech Republic');");
    
    // vacuum SQLite database file to avoid its excessive growth
    db.vacuum();

    // close connection to SQLite database file
    db.close();

    // open connection to SQLite database file in read-only mode
    db.open(db_file_nm, true);

    // query database
    rslt = db.query(sql);

    // print result of SQL query
    for (int i = 0; i < (*rslt).data.values.size(); i++)
    {
        for (int j = 0; j < (*rslt).data.values[0].size(); j++)
        {
            cout << (*rslt).data.values[i][j] << " ";
        }
        cout << '\n';
    }

    // close connection to SQLite database file
    db.close();

    // delete pointer
    delete rslt;

    // everything OK
    return 0;
*/

// define object that handles SQLite database
class mySQLite
{
    private:
        // SQLite database
        sqlite3 *db;

    public:
        // how many seconds to wait for SQLite database file being available
        int wait_max_seconds;

        // object constructors
        mySQLite(const char *db_file_nm, const bool read_only, int wait_max_seconds);

        // object destructor
        ~mySQLite(){};

        // object function declarations
        void open(const char *db_file_nm, const bool read_only);
        void close();
        void vacuum();
        void exec(const std::string &sql);
        myDataFrame * query(const std::string &sql);
        myDataFrame * download_tbl(const std::string &tbl_nm);
        void upload_tbl(const myDataFrame &tbl, const std::string &tbl_nm, const bool delete_old_data);
};