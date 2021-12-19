#include <iostream>
#include <stdio.h>
#include <string>
#include <cstring>
#include <sqlite3.h>
#include <chrono>
#include <thread>
#include <fstream>
#include "lib_aux.h"
#include "lib_dataframe.h"
#include "lib_sqlite.h"

/*
 * OBJECT CONSTRUCTORS
 */

mySQLite::mySQLite(const char *db_file_nm, const bool read_only, int wait_max_seconds)
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
    int counter = wait_max_seconds;

    // wait for the SQLite database file being available
    if (read_only)
    {
        do
        {
            sts = sqlite3_open_v2(db_file_nm, &db, SQLITE_OPEN_READONLY, NULL);
            std::this_thread::sleep_for(std::chrono::seconds(1));
            counter--;

            if (counter == 0)
            {
                throw std::runtime_error((std::string)__func__ + ": SQLite database file is locked for more than " + std::to_string(wait_max_seconds) + " seconds!");
            }

        }
        while ((sts == SQLITE_BUSY) && (counter > 0)); // wait for "free" SQLite database file
    }
    else
    {
        do
        {
            sts = sqlite3_open_v2(db_file_nm, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
            std::this_thread::sleep_for(std::chrono::seconds(1));
            counter--;

            if (counter == 0)
            {
                throw std::runtime_error((std::string)__func__ + ": SQLite database file is locked for more than " + std::to_string(wait_max_seconds) + " seconds!");
            }

        }
        while ((sts == SQLITE_BUSY) && (counter > 0)); // wait for "free" SQLite database file
    }
	
    // check everything is OK and throw an error if not
    if (sts != SQLITE_OK)
    {
        throw std::runtime_error((std::string)__func__ + ": " + sqlite3_errstr(sts));
    }
}

// close SQLite database file
void mySQLite::close() const
{
    // SQLite status code
    int sts;
    int counter = this->wait_max_seconds;

    // try to close the database
    do
    {
        sts = sqlite3_close(db);
        std::this_thread::sleep_for(std::chrono::seconds(1));
        counter--;

        if (counter == 0)
        {
            throw std::runtime_error("SQLite database file is locked for more than " + std::to_string(wait_max_seconds) + " seconds!");
        }
    }
    while ((sts == SQLITE_BUSY) && (counter > 0)); // wait for "free" SQLite database file
    
    // check everything is OK and throw an error if not 
    if (sts != SQLITE_OK)
    {
        throw std::runtime_error((std::string)__func__ + ": " + sqlite3_errstr(sts));
    }
}

// vaccum SQLite database file to avoid it excessive growth
void mySQLite::vacuum() const
{
    // SQLite status code
    int sts;
    int counter = this->wait_max_seconds;

    // vacuum SQLite database file
    do
    {
        sts = sqlite3_exec(db, "VACUUM;", NULL, NULL, NULL);
        std::this_thread::sleep_for(std::chrono::seconds(1));
        counter--;

        if (counter == 0)
        {
            throw std::runtime_error("SQLite database file is locked for more than " + std::to_string(wait_max_seconds) + " seconds!");
        }

    }
    while ((sts == SQLITE_BUSY) && (counter > 0)); // wait for "free" SQLite database file
    
    // check everything is OK and throw an error if not 
    if (sts != SQLITE_OK)
    {
        throw std::runtime_error((std::string)__func__ + ": " + sqlite3_errstr(sts));
    }
}

// execute SQL command (INSERT, DELETE, UPDATE)
void mySQLite::exec(const std::string &sql) const
{
    // SQLite status code
    int sts;
    int counter = this->wait_max_seconds;

    // execute SQL command
    do
    {
        sts = sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL);
        std::this_thread::sleep_for(std::chrono::seconds(1));
        counter--;

        if (counter == 0)
        {
            throw std::runtime_error((std::string)__func__ + ": SQLite database file is locked for more than " + std::to_string(wait_max_seconds) + " seconds!");
        }

    }
    while ((sts == SQLITE_BUSY) && (counter > 0)); // wait for "free" SQLite database file
    
    // check everything is OK and throw an error if not 
    if (sts != SQLITE_OK)
    {
        std::cout << sql << std::endl;
        throw std::runtime_error((std::string)__func__ + ": " + sqlite3_errstr(sts));
    }
}

// check column type
bool check_type(sqlite3_stmt * stmt, myDataFrame * rslt, int cols_no)
{
    // information if all column types are already determined
    bool all_col_dtypes_determined = true;

    // date type of dataframe column
    int col_dtype;

    // go column by column and update its type if NULL
    for (int col_idx = 0; col_idx < cols_no; col_idx++)
    {
        if (rslt->tbl.dtypes[col_idx].compare("NULL"))
        {
            all_col_dtypes_determined = false;
            col_dtype = sqlite3_column_type(stmt, col_idx);
            switch (col_dtype)
            {
                case SQLITE_NULL:
                    break;
                case SQLITE_INTEGER:
                    rslt->tbl.dtypes[col_idx] = "INT";
                    break;
                case SQLITE_FLOAT:
                    rslt->tbl.dtypes.push_back("FLOAT");
                    break;
                case SQLITE_TEXT:
                    (*rslt).tbl.dtypes.push_back("CHAR");
                    break;                    
                case SQLITE_BLOB:
                    throw std::invalid_argument((std::string)__func__ + ": Unsupported SQL data type SQLITE_BLOB in column " + (*rslt).tbl.col_nms[col_idx] + "!");
                default:
                    throw std::invalid_argument((std::string)__func__ + ": Unsupported SQL data type with code " + std::to_string(col_dtype) + " in column " + (*rslt).tbl.col_nms[col_idx] + "!");
            }
        }
        
    }

    // return boolean indicating if all column types have been determined
    return all_col_dtypes_determined;
}

// add row to SQL query result
void add_row(sqlite3_stmt * stmt, myDataFrame * rslt, int cols_no)
{
    // std::vector of std::strings to store data in
    std::vector<std::string> row;

    // go column by column
    for (int col_idx = 0; col_idx < cols_no; col_idx++)
    {
        if ((char*)sqlite3_column_text(stmt, col_idx) == NULL)
            row.push_back("");
        else
            row.push_back((char*)sqlite3_column_text(stmt, col_idx));
    }

    // add row to SQL query result
    (*rslt).tbl.values.push_back(row);
}

// execute SQL query (SELECT)
myDataFrame * mySQLite::query(const std::string &sql) const
{
    // SQLite status code
    int sts;
    int counter = wait_max_seconds;

    // SQLite statement
    sqlite3_stmt *stmt = nullptr;

    // dataframe
    myDataFrame *rslt = new myDataFrame();

    // number of columns in dataFrame
    int cols_no;



    // data row of dataframe
    std::vector<std::string> row;

    // initiate SQL query
    do
    {
        sts = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);
        std::this_thread::sleep_for(std::chrono::seconds(1));
        counter--;

        if (counter == 0)
        {
            throw std::runtime_error((std::string)__func__ + ": SQLite database file is locked for more than " + std::to_string(wait_max_seconds) + " seconds!");
        }

    }
    while ((sts == SQLITE_BUSY) && (counter > 0)); // wait for "free" SQLite database file

    // check everything is OK and throw an error if not 
    if (sts != SQLITE_OK)
    {
        sqlite3_finalize(stmt);
        std::cout << sql << std::endl;
        throw std::runtime_error((std::string)__func__ + ": " + sqlite3_errstr(sts));
    }

    // get number of columns
    cols_no = sqlite3_column_count(stmt);

    // get column names from the first row of the dataframe
    for (int col_idx = 0; col_idx < cols_no; col_idx++)
    {
        rslt->tbl.col_nms.push_back((char*)sqlite3_column_name(stmt, col_idx));
    }

    // initiate column data types
    for (int col_idx = 0; col_idx < cols_no; col_idx++)
    {
        rslt->tbl.dtypes.push_back("NULL");
    }

    // add rows of SQL query result and update column data types
    bool all_col_dtypes_determined = false;
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        if (!all_col_dtypes_determined)
        {
            all_col_dtypes_determined = check_type(stmt, rslt, cols_no);
        }

        add_row(stmt, rslt, cols_no);
    }

    // convert column data type NULL to CHAR
    for (int col_idx = 0; col_idx < cols_no; col_idx++)
    {
        if (rslt->tbl.dtypes[col_idx].compare("NULL") == 0)
        {
            rslt->tbl.dtypes[col_idx] = "CHAR";
        }
    }

    // delete the statement
    sqlite3_finalize(stmt);

    // return query result
    return rslt;
}

// download table from SQLite database file
myDataFrame * mySQLite::download_tbl(const std::string &tbl_nm)
{
    // dataframe
    myDataFrame *rslt = new myDataFrame();
    rslt = this->query("SELECT * FROM " + tbl_nm + ";");
    return rslt;
}

// upload table into SQLite database file
void mySQLite::upload_tbl(const myDataFrame &tbl, const std::string &tbl_nm, const bool delete_old_data)
{
    // variables
    int cols_no;
    long rows_no;
    int col_idx;
    std::string col_nm;
    std::string dtype;
    std::string sql;
    std::string sql_insert_row;
    std::string sql_insert_row_tmpl;
    std::string col_allias;
    std::string col_val;
    int col_pos;
    int col_len;

    // create table if it does not exist

        // get number of columns
        cols_no = tbl.get_cols_no();
        
        // create SQL
        sql = "CREATE TABLE IF NOT EXISTS " + tbl_nm + " (";
        for (col_idx = 0; col_idx < cols_no; col_idx++)
        {
            // get column name and column type
            col_nm = tbl.tbl.col_nms[col_idx];
            dtype = to_upper(tbl.tbl.dtypes[col_idx]);

            // add column name and its type
            if (dtype.compare("INT") == 0 ||
                dtype.compare("FLOAT") == 0 ||
                dtype.compare("INT") == 0 ||
                dtype.compare("CHAR") == 0 ||
                dtype.compare("VARCHAR") == 0)
            {
                sql += col_nm + " " + dtype + ", ";
            }
            else
            {
                throw std::invalid_argument((std::string)__func__ + ": Unsupported SQL data type " + dtype + " in column " + col_nm + "!");
            }
        }

        sql = sql.substr(0, sql.size() - 2);
        sql += ");";

    // execute SQL
    this->exec(sql);

    // delete old data if asked so
    if (delete_old_data)
    {
        this->exec("DELETE FROM " + tbl_nm + ";");
    }

    // create list of inserts

        // get number of rows
        rows_no = tbl.get_rows_no();

        // prepare insert
        sql = "BEGIN TRANSACTION;";
        sql +='\n';

        // prepare single row INSERT template
        sql_insert_row_tmpl = "INSERT INTO " + tbl_nm + " (";
        for (int col_idx = 0; col_idx < cols_no; col_idx++)
        {
            sql_insert_row_tmpl += tbl.tbl.col_nms[col_idx] + ", "; 
        }
        sql_insert_row_tmpl = sql_insert_row_tmpl.substr(0, sql_insert_row_tmpl.size() - 2);
        sql_insert_row_tmpl += ") VALUES (";
        for (int col_idx = 0; col_idx < cols_no; col_idx++)
        {
            col_allias = "col_idx_" + std::to_string(col_idx);
            sql_insert_row_tmpl += col_allias + ", ";
        }
        sql_insert_row_tmpl = sql_insert_row_tmpl.substr(0, sql_insert_row_tmpl.size() - 2);
        sql_insert_row_tmpl += ");";

        // go row by row and use the INSERT template
        for (int row_idx = 0; row_idx < rows_no; row_idx++)
        {
            sql_insert_row = sql_insert_row_tmpl;
            for (int col_idx = 0; col_idx < cols_no; col_idx++)
            {
                // NULL
                if (tbl.tbl.values[row_idx][col_idx].compare("") == 0)
                {
                    col_val = "NULL";
                }
                // text column value
                else if ((tbl.tbl.dtypes[col_idx].compare("CHAR") == 0) || (tbl.tbl.dtypes[col_idx].compare("VARCHAR") == 0))
                {
                    col_val = "'" + tbl.tbl.values[row_idx][col_idx] + "'";
                }
                // numerical column value (unsupport data types were address above)
                else
                {
                    col_val = tbl.tbl.values[row_idx][col_idx];
                }

                // replace column allias with column value
                col_allias = "col_idx_" + std::to_string(col_idx);
                col_pos = sql_insert_row.find(col_allias);
                col_len = col_allias.size();
                sql_insert_row.replace(col_pos, col_len, col_val);
            }

            // add INSERT row
            sql += sql_insert_row;
            sql += '\n';
        }
        sql += "COMMIT;";
        sql += '\n';

        // now we have INSERT statement ready, insert all the rows into SQLite database file
        this->exec(sql);
}

/*
 * STANDALONE FUNCTIONS
 */

std::string read_sql(std::string sql_file_nm, std::string tag)
{
    // declare file stream
    std::ifstream f;

    // varible holding line
    std::string line;

    // std::string holding tag
    std::string tag_aux = "###!";
    tag = tag_aux + tag;

    // boolean variable indicating begining of SQL query we are looking for
    bool sql_query_found = false;

    // std::string variable that hold SQL query we want to extract
    std::string sql = "";

    f.open(sql_file_nm, std::ios::out);
    if (f.is_open())
    {
        while(getline(f, line))
        {
            // check that we have not reached begining of another SQL query
            if (sql_query_found && (line.substr(0, 4).compare(tag_aux) == 0))
            {
                // check that SQL query has been loaded?
                if (sql.compare("") == 0)
                {
                    throw std::runtime_error((std::string)__func__ + ": Tag " + tag + " not found in file " + sql_file_nm + "!");
                }

                // we have reached another SQL query, so return what you have read so far
                return sql;
            }

            // add the line to SQL query if eligible
            if (sql_query_found)
            {
                sql += line;
            }

            // check if hit the required SQL query
            if (line.substr(0,tag.length()).compare(tag) == 0)
                sql_query_found = true;   
        }

        // close file with SQL queries
        f.close();

        // check that SQL query has been loaded?
        if (sql.compare("") == 0)
        {
            throw std::runtime_error((std::string)__func__ + ": Tag " + tag + " not found in file " + sql_file_nm + "!");
        }

        // we have reached end of the file, so return what you have read so far
        return sql;
    }
    else
    {
        throw std::runtime_error((std::string)__func__ + ": Unable to open file " + sql_file_nm + "!");
    }
}

// make substitutions in SQL query
std::string replace_in_sql(std::string sql, std::string replace_what, std::string replace_with)
{
    // find position of replace_what std::string
    size_t pos = sql.find(replace_what);

    // check that replace_what is present in SQL query
    if (pos == std::string::npos)
    {
        throw std::invalid_argument((std::string)__func__ + ": " + replace_what + " is  is not present in SQL query!");
    }

    // replace replace_what with replace_with
    sql = sql.replace(pos, replace_what.size(), replace_with);

    // return the adjust SQL query
    return sql;
}
