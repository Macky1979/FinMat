/*
#include <string>
#include <iostream>
#include <vector>
#include <sqlite3.h>
#include <memory>
#include "lib_aux.h"
#include "lib_dataframe.h"
#include "lib_sqlite.h"
#include "fin_swaption.h"

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
    std::string vol_surf_def = "data/vol_surf_def.csv";
    std::string vol_surf_data = "data/vol_surf_data.csv";
    std::string cap_floor_data = "data/swpt_data.csv";
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

    sql = read_sql(sql_file_nm, "vol_surf_def");
    db.exec(sql);

    sql = read_sql(sql_file_nm, "vol_surf_data");
    db.exec(sql);

    sql = read_sql(sql_file_nm, "swaption_data");
    db.exec(sql);

    sql = read_sql(sql_file_nm, "swaption_npv");
    db.exec(sql);

    // delete old content in the tables
    db.exec("DELETE FROM cnty_def;");
    db.exec("DELETE FROM ccy_def;");
    db.exec("DELETE FROM ccy_data;");
    db.exec("DELETE FROM freq_def;");
    db.exec("DELETE FROM dcm_def;");
    db.exec("DELETE FROM crv_def;");
    db.exec("DELETE FROM crv_data;");
    db.exec("DELETE FROM vol_surf_def;");
    db.exec("DELETE FROM vol_surf_data;");
    db.exec("DELETE FROM swaption_data;");
    db.exec("DELETE FROM swaption_npv;");

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
   
    rslt->read(vol_surf_def, sep, quotes);
    db.upload_tbl(*rslt, "vol_surf_def", delete_old_data);
    rslt->clear();

    rslt->read(vol_surf_data, sep, quotes);
    db.upload_tbl(*rslt, "vol_surf_data", delete_old_data);
    rslt->clear();

    rslt->read(cap_floor_data, sep, quotes);
    db.upload_tbl(*rslt, "swaption_data", delete_old_data);
    rslt->clear();

    // vacuum SQLite database file to avoid its excessive growth
    db.vacuum();

    std::cout << get_timestamp() + " - initiating curves, FX rates and volatility surfaces..." << std::endl;

    // load FX rates
    myFx fx = myFx(db, sql_file_nm);

    // load all curves
    myCurves crvs = myCurves(db, sql_file_nm, calc_date);

    // load all volatility surfaces
    myVolSurfaces vol_surfs = myVolSurfaces(db, sql_file_nm);

    std::cout << get_timestamp() + " - initiating swaptions..." << std::endl;

    // define entity and portfolio
    std::string ent_nm = "kbc";
    std::string ptf = "swpt";

    // load swaptions
    mySwaptions swpts = mySwaptions(db, "SELECT * FROM swaption_data WHERE ent_nm = '" + ent_nm + "' AND ptf = '" + ptf + "';", calc_date);

    // define scenario number and reference currency to be used in valuation
    int scn_no = 1;
    std::string ref_ccy_nm = "EUR";

    std::cout << get_timestamp() + " - evaluating swaptions on a single core..." << std::endl;

    // evaluate swaptions using single core
    swpts.calc_npv(scn_no, crvs, fx, vol_surfs, ref_ccy_nm);
    
    std::cout << get_timestamp() + " - evaluating swaptions using multithreading..." << std::endl;

    // evaluate swaptions using multiple cores
    int threads_no = 2;

    std::cout << get_timestamp() + " -    spliting contracts..." << std::endl;

    std::vector<mySwaptions> swpts_thrd = swpts.split(threads_no);

    std::cout << get_timestamp() + " -    running individual threads..." << std::endl;

    std::vector<std::thread> workers;
    for (int thread_idx = 0; thread_idx < threads_no; thread_idx++)
    {
        workers.emplace_back(swpts_thrd[thread_idx].calc_npv_thrd(scn_no, crvs, fx, vol_surfs, ref_ccy_nm));
    }

    std::cout << get_timestamp() + " -    waiting for individual threads to finish..." << std::endl;

    for (int thread_idx = 0; thread_idx < threads_no; thread_idx++)
    {
        workers[thread_idx].join();
    }

    std::cout << get_timestamp() + " -    merging results..." << std::endl;

    // merge results into a single vector
    swpts.merge(swpts_thrd);

    std::cout << get_timestamp() + " - storing NPV into SQLite database file..." << std::endl;

    // store results
    swpts.write_npv(db, scn_no, ent_nm, ptf);

    std::cout << get_timestamp() + " - closing SQLite database file..." << std::endl;

    // close connection to SQLite database file
    db.close();

    std::cout << get_timestamp() + " - done!" << std::endl;
    
    // everything OK
    return 0;
}
*/

# pragma once

#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include "lib_date.h"
#include "fin_curve.h"
#include "fin_fx.h"
#include "fin_vol_surf.h"

// event data type
struct swpt_event
{
    myDate date_begin;
    myDate date_end;
    double nominal_begin = 0.0;
    bool amort_flg = false;
    double amort_payment = 0.0;
    double nominal_end = 0.0;
    double int_year_frac = 0.0;
    double fwd = 0.0;
    double df = 0.0;
};

// swaption data
struct swpt_info
{
    std::string ent_nm;
	std::string parent_id;
	std::string contract_id;
	std::string ptf;
    std::string issuer_id;
    std::string account;
	std::string isin;
	std::string rtg;
    std::string comments;
    std::string swaption_type;
	std::string ccy_nm;
	double nominal = 0.0;
    myDate calc_date;
	myDate value_date;
	myDate maturity_date;
    std::string dcm;
    double swaption_rate = 0.0;
    double swap_rate = 0.0;
    std::string swaption_vol_surf;
    double swaption_vol = 0.0;
    std::string fix_freq;
	myDate first_amort_date;
	std::string amort_freq;
	double amort = 0.0;
	std::string crv_disc;
	std::string crv_fwd;
    double d = 0.0;
    double aux1 = 0.0;
    double aux2 = 0.0;
    double aux3 = 0.0;
    double aux4 = 0.0;
    double flt_leg_npv = 0.0;
    double npv = 0.0;
    double npv_ref_ccy = 0.0;
    std::string wrn_msg = "";
    std::vector<swpt_event> events;
};

/*
 * SWAPTION CLASS
 */

class mySwaptions
{
    private:
        // variables
        std::vector<swpt_info> info;

    public:
        // object constructors
        mySwaptions(std::vector<swpt_info> info){this->info = info;};
        mySwaptions(const mySQLite &db, const std::string &sql, const myDate &calc_date);

        // copy constructor
        mySwaptions(const mySwaptions &swpts){this->info = swpts.info;};

        // object destructors
        ~mySwaptions(){};

        // object function declarations
        void clear(){this->info.clear();};
        std::vector<mySwaptions> split(const int &threads_no);
        void merge(std::vector<mySwaptions> &swpts);
        void calc_npv(const int &scn_no, const myCurves &crvs, const myFx &fx, const myVolSurfaces &vol_surfs, const std::string &ref_ccy_nm);
        std::thread calc_npv_thrd(const int &scn_no, const myCurves &crvs, const myFx &fx, const myVolSurfaces &vol_surfs, const std::string &ref_ccy_nm);
        void write_npv(mySQLite &db, const int &scn_no, const std::string &ent_nm, const std::string &ptf);
};
