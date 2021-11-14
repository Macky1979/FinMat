#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <sqlite3.h>

// user defined datatype to hold result of SQL query
struct sqlQueryResult
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
        sqlQueryResult * query(const std::string &sql);
        void upload_tbl(const sqlQueryResult &tbl, const std::string &tbl_nm);
        sqlQueryResult * download_tbl(const std::string &tbl_nm);

};