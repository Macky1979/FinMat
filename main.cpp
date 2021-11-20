#include <string>
#include <iostream>
#include <vector>
#include <sqlite3.h>
#include "lib_dataframe.h"
#include "lib_sqlite.h"
#include "lib_aux.h"

using namespace std;

int main()
{
    // variables
    const char * db_file_nm = "database.db";
    string file_nm = "cities.csv";
    string file_nm2 = "cities2.csv";
    myDataFrame * rslt = new myDataFrame();
    myDataFrame * rslt2 = new myDataFrame();
    bool read_only = false;
    int wait_max_seconds = 10;
    bool delete_old_data = true;
    string sep = ";";
    bool quotes = true;

    // create SQLite object and open connection to SQLite database file in read-write mode
    mySQLite db(db_file_nm, read_only, wait_max_seconds);

    // create table if it does not exists
    db.exec("CREATE TABLE IF NOT EXISTS cities (city VARCHAR(20), country VARCHAR(20));");

    // delete table
    db.exec("DELETE FROM cities;");   

    // insert data into table
    db.exec("INSERT INTO cities (city, country) VALUES ('Prague', 'Czech Republic');");
    
    // vacuum SQLite database file to avoid its excessive growth
    db.vacuum();

    // download table
    rslt = db.download_tbl("cities");

    // close connection to SQLite database file
    db.close();

    // write dataframe into a .csv file
    rslt->write(file_nm, sep, quotes);

    // create dataframe from a .csv file
    rslt2->read(file_nm, sep, quotes);
    rslt2->write(file_nm2, sep, quotes);

    // delete pointers
    delete rslt;
    delete rslt2;

    // everything OK
    return 0;
}