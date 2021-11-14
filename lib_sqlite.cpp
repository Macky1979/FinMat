#include <iostream>
#include <stdio.h>
#include <string>
#include <sqlite3.h>
#include <chrono>
#include <thread>
#include "lib_sqlite.h"

using namespace std;

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
    sqlQueryResult * rslt = new sqlQueryResult;

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

/*
 * OBJECT CONSTRUCTORS
 */

mySQLite::mySQLite(const char *db_file_nm, const bool read_only)
{
    // open SQLite database file name
    this->open(db_file_nm, read_only);
}

/*
 * OBJECT FUNCTIONS
 */

// open SQLite database file
void mySQLite::open(const char *db_file_nm, const bool read_only)
{
    // SQLite status code
    int sts;

    // open SQLite database file
    if (read_only)
    {
        do
        {
            sts = sqlite3_open_v2(db_file_nm, &db, SQLITE_OPEN_READONLY, NULL);
            this_thread::sleep_for(chrono::seconds(1));
        }
        while (sts == SQLITE_BUSY); // wait for "free" SQLite database file
    }
    else
    {
        do
        {
            sts = sqlite3_open_v2(db_file_nm, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
            this_thread::sleep_for(chrono::seconds(1));
        }
        while (sts == SQLITE_BUSY); // wait for "free" SQLite database file
    }
	
    // check everything is OK and throw an error if not
    if (sts != SQLITE_OK)
    {
        throw std::runtime_error(sqlite3_errstr(sts));
    }
}

// close SQLite database file
void mySQLite::close()
{
    // SQLite status code
    int sts;

    // try to close the database
    do
    {
        sts = sqlite3_close(db);
        this_thread::sleep_for(chrono::seconds(1));
    }
    while (sts == SQLITE_BUSY); // wait for "free" SQLite database file
    
    // check everything is OK and throw an error if not 
    if (sts != SQLITE_OK)
    {
        throw std::runtime_error(sqlite3_errstr(sts));
    }
}

// vaccum SQLite database file to avoid it excessive growth
void mySQLite::vacuum()
{
    // SQLite status code
    int sts;

    // vacuum SQLite database file
    do
    {
        sts = sqlite3_exec(db, "VACUUM;", NULL, NULL, NULL);
        this_thread::sleep_for(chrono::seconds(1));
    }
    while (sts == SQLITE_BUSY); // wait for "free" SQLite database file
    
    // check everything is OK and throw an error if not 
    if (sts != SQLITE_OK)
    {
        throw std::runtime_error(sqlite3_errstr(sts));
    }
}

// execute SQL command (INSERT, DELETE, UPDATE)
void mySQLite::exec(const string &sql)
{
    // SQLite status code
    int sts;

    // execute SQL command
    do
    {
        sts = sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL);
        this_thread::sleep_for(chrono::seconds(1));
    }
    while (sts == SQLITE_BUSY); // wait for "free" SQLite database file
    
    // check everything is OK and throw an error if not 
    if (sts != SQLITE_OK)
    {
        throw std::runtime_error(sqlite3_errstr(sts));
    }
}

// add row to SQL query result
void add_row(sqlite3_stmt *stmt, sqlQueryResult * rslt, int cols_no)
{
    // vector of strings to store data in
    vector<string> row;

    // go column by column
    for (int col_idx = 0; col_idx < cols_no; col_idx++)
    {
        if ((char*)sqlite3_column_text(stmt, col_idx) == NULL)
            row.push_back("");
        else
            row.push_back((char*)sqlite3_column_text(stmt, col_idx));
    }

    // add row to SQL query result
    (*rslt).values.push_back(row);
}

// execute SQL query (SELECT)
sqlQueryResult * mySQLite::query(const string &sql)
{
    // SQLite status code
    int sts;

    // SQLite statement
    sqlite3_stmt *stmt = nullptr;

    // record set
    sqlQueryResult *rslt = new sqlQueryResult;

    // number of columns in sqlQueryResult
    int cols_no;

    // date type of sqlQueryResult column
    int col_dtype;

    // data row of sqlQueryResult
    vector<string> row;

    // initiate SQL query
    do
    {
        sts = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);
        this_thread::sleep_for(chrono::seconds(1));
    }
    while (sts == SQLITE_BUSY); // wait for "free" SQLite database file

    // check everything is OK and throw an error if not 
    if (sts != SQLITE_OK)
    {
        sqlite3_finalize(stmt);
        throw std::runtime_error(sqlite3_errstr(sts));
    }

    // get number of columns
    cols_no = sqlite3_column_count(stmt);

    // get column names from the first row of the result set
    for (int col_idx = 0; col_idx < cols_no; col_idx++)
    {
        (*rslt).col_nms.push_back((char*)sqlite3_column_name(stmt, col_idx));
    }

    // the first row of SQL query result
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        // get column type - SQLite does not impose strict datatype requirements => we assume that the first row is representative 
        for (int col_idx = 0; col_idx < cols_no; col_idx++)
        {
            col_dtype = sqlite3_column_type(stmt, col_idx);
            switch (col_dtype)
            {
                case SQLITE_INTEGER : (*rslt).dtypes.push_back("INT");
                    break;
                case SQLITE_FLOAT : (*rslt).dtypes.push_back("FLOAT");
                    break;
                case SQLITE_TEXT : (*rslt).dtypes.push_back("CHAR");
                    break;
                default : (*rslt).dtypes.push_back("N/A");
                    break;
            }
        }

        // add the first row to SQL query result
        add_row(stmt, rslt, cols_no);
    }

    // add the following rows of SQL query result
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        add_row(stmt, rslt, cols_no);
    }

    // delete the statement
    sqlite3_finalize(stmt);

    // return query result
    return rslt;
}
