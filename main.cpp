
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