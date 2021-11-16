#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <sqlite3.h>

/*
#include <string>
#include <iostream>
#include <sqlite3.h>
#include "lib_sqlite.h"

using namespace std;

int main()
{
    // variables
    const char * db_file_nm = "database.db";
    string sql = "SELECT * FROM cities;";
    dataFrame * rslt = new dataFrame;

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
    for (int i = 0; i < (*rslt).values.size(); i++)
    {
        for (int j = 0; j < (*rslt).values[0].size(); j++)
        {
            cout << (*rslt).values[i][j] << " ";
        }
        cout << '\n';
    }

    // close connection to SQLite database file
    db.close();

    // delete pointer
    delete rslt;

    // everything OK
    return 0;
}
*/

// user defined datatype to hold result of SQL query
struct dataFrame
{
	std::vector<std::string> col_nms;
	std::vector<std::string> dtypes;
	std::vector<std::vector<std::string>> values; 
};

// define object that handles SQLite database
class mySQLite
{
    private:
        // SQLite database
        sqlite3 *db;

    public:
        // object constructors
        mySQLite(const char *db_file_nm, const bool read_only);

        // object destructor
        ~mySQLite(){};

        // object function declarations
        void open(const char *db_file_nm, const bool read_only);
        void close();
        void vacuum();
        void exec(const std::string &sql);
        dataFrame * query(const std::string &sql);
        void upload_tbl(const dataFrame &tbl, const std::string &tbl_nm);
        dataFrame * download_tbl(const std::string &tbl_nm);

};