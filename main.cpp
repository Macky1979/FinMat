#include <string>
#include <iostream>
#include <vector>
#include <sqlite3.h>
#include <memory>
#include "lib_aux.h"
#include "lib_dataframe.h"
#include "lib_sqlite.h"
#include "fin_fx.h"
#include "fin_bond.h"

int main()
{
    std::cout << get_timestamp() + " - loading data..." << std::endl;

    // variables
    const char * db_file_nm = "data/finmat.db";
    std::string sql_file_nm = "data/finmat.sql";
    std::string cnty_def = "data/cnty_def.csv";
    std::string ccy_def = "data/ccy_def.csv";
    std::string ccy_data = "data/ccy_data.csv";
    std::string freq_def = "data/freq_def.csv";
    std::string dcm_def = "data/dcm_def.csv";
    std::string crv_def = "data/crv_def.csv";
    std::string interbcrv_eur = "data/curves/interbcrv_eur.csv";
    std::string spreadcrv_bef = "data/curves/spreadcrv_bef.csv";
    std::string bnd_data = "data/bnd_data.csv";
    std::string sql;
    myDataFrame * rslt = new myDataFrame();
    myDate calc_date = myDate(20211203);
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

    sql = read_sql(sql_file_nm, "freq_def");
    db.exec(sql);

    sql = read_sql(sql_file_nm, "dcm_def");
    db.exec(sql);

    sql = read_sql(sql_file_nm, "crv_def");
    db.exec(sql);

    sql = read_sql(sql_file_nm, "crv_data");
    db.exec(sql);

    sql = read_sql(sql_file_nm, "bnd_data");
    db.exec(sql);

    sql = read_sql(sql_file_nm, "bnd_npv");
    db.exec(sql);

    // delete old content in the tables
    db.exec("DELETE FROM cnty_def;");
    db.exec("DELETE FROM ccy_def;");
    db.exec("DELETE FROM ccy_data;");
    db.exec("DELETE FROM freq_def;");
    db.exec("DELETE FROM dcm_def;");
    db.exec("DELETE FROM crv_def;");
    db.exec("DELETE FROM crv_data;");
    db.exec("DELETE FROM bnd_data;");
    db.exec("DELETE FROM bnd_npv;");

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

    rslt->read(freq_def, sep, quotes);
    db.upload_tbl(*rslt, "freq_def", delete_old_data);
    rslt->clear();
    
    rslt->read(dcm_def, sep, quotes);
    db.upload_tbl(*rslt, "dcm_def", delete_old_data);
    rslt->clear();

    rslt->read(crv_def, sep, quotes);
    db.upload_tbl(*rslt, "crv_def", delete_old_data);
    rslt->clear();

    rslt->read(interbcrv_eur, sep, quotes);
    db.upload_tbl(*rslt, "crv_data", delete_old_data);
    rslt->clear();
   
    rslt->read(spreadcrv_bef, sep, quotes);
    db.upload_tbl(*rslt, "crv_data", delete_old_data);
    rslt->clear();

    rslt->read(bnd_data, sep, quotes);
    db.upload_tbl(*rslt, "bnd_data", delete_old_data);
    rslt->clear();

    // vacuum SQLite database file to avoid its excessive growth
    db.vacuum();

    std::cout << get_timestamp() + " - initiating curves and FX rates..." << std::endl;

    // load FX rates
    myFx fx = myFx(db, sql_file_nm);

    // load all curves
    myCurves crvs = myCurves(db, sql_file_nm, calc_date);

    std::cout << get_timestamp() + " - initiating bonds..." << std::endl;

    // define entity and portfolio
    std::string ent_nm = "kbc";
    std::string ptf = "bnd";

    // load bonds
    myBonds bnds = myBonds(db, "SELECT * FROM bnd_data WHERE ent_nm = '" + ent_nm + "' AND ptf = '" + ptf + "';", calc_date);

    // define scenario number and reference currency to be used in valuation
    int scn_no = 1;
    std::string ref_ccy_nm = "EUR";

    std::cout << get_timestamp() + " - evaluating bonds on a single core..." << std::endl;

    // evaluate bonds using single core
    bnds.calc_npv(scn_no, crvs, fx, ref_ccy_nm);

    std::cout << get_timestamp() + " - evaluating bonds using multithreading..." << std::endl;

    // evaluate bonds using multiple cores
    int threads_no = 4;

    std::cout << get_timestamp() + " -    spliting contracts..." << std::endl;

    std::vector<myBonds> bnds_thrd = bnds.split(threads_no);

    std::cout << get_timestamp() + " -    running individual threads..." << std::endl;

    std::vector<std::thread> workers;
    for (int thread_idx = 0; thread_idx < threads_no; thread_idx++)
    {
        workers.emplace_back(bnds_thrd[thread_idx].calc_npv_thrd(scn_no, crvs, fx, ref_ccy_nm));
    }

    std::cout << get_timestamp() + " -    waiting for individual threads to finish..." << std::endl;

    for (int thread_idx = 0; thread_idx < threads_no; thread_idx++)
    {
        workers[thread_idx].join();
    }

    std::cout << get_timestamp() + " -    merging results..." << std::endl;

    // merge results into a single vector
    bnds.merge(bnds_thrd);

    std::cout << get_timestamp() + " - storing NPV into SQLite database file..." << std::endl;

    // store results
    bnds.write_npv(db, scn_no, ent_nm, ptf);

    std::cout << get_timestamp() + " - closing SQLite database file..." << std::endl;

    // close connection to SQLite database file
    db.close();

    std::cout << get_timestamp() + " - done!" << std::endl;
    
    // everything OK
    return 0;
}