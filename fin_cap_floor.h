/*
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
    myCapsFloors caps_flrs = myCapsFloors(db, "SELECT * FROM bnd_data WHERE ent_nm = '" + ent_nm + "' AND ptf = '" + ptf + "';", calc_date);

    // define scenario number and reference currency to be used in valuation
    int scn_no = 1;
    std::string ref_ccy_nm = "EUR";

    std::cout << get_timestamp() + " - evaluating bonds on a single core..." << std::endl;

    // evaluate bonds using single core
    caps_flrs.calc_npv(scn_no, crvs, fx, ref_ccy_nm);

    std::cout << get_timestamp() + " - evaluating bonds using multithreading..." << std::endl;

    // evaluate annuties using multiple cores
    int threads_no = 4;

    std::cout << get_timestamp() + " -    spliting contracts..." << std::endl;

    std::vector<myCapsFloors> caps_flrs_thrd = caps_flrs.split(threads_no);

    std::cout << get_timestamp() + " -    running individual threads..." << std::endl;

    std::vector<std::thread> workers;
    for (int thread_idx = 0; thread_idx < threads_no; thread_idx++)
    {
        workers.emplace_back(caps_flrs_thrd[thread_idx].calc_npv_thrd(scn_no, crvs, fx, ref_ccy_nm));
    }

    std::cout << get_timestamp() + " -    waiting for individual threads to finish..." << std::endl;

    for (int thread_idx = 0; thread_idx < threads_no; thread_idx++)
    {
        workers[thread_idx].join();
    }

    std::cout << get_timestamp() + " -    merging results..." << std::endl;

    // merge results into a single vector
    caps_flrs.merge(caps_flrs_thrd);

    std::cout << get_timestamp() + " - storing NPV into SQLite database file..." << std::endl;

    // store results
    caps_flrs.write_npv(db, scn_no, ent_nm, ptf);

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

// event data type
struct cap_flr_event
{
    myDate date_begin;
    myDate date_end;
    double nominal_begin = 0.0;
    bool amort_flg = false;
    double amort_payment = 0.0;
    double nominal_end = 0.0;
    bool is_cpn_payment = false;
    double cpn = 0.0;
    bool fix_flg = false;
    bool is_cpn_fixed = false;
    double cpn_year_frac = 0.0;
    double df = 0.0;
    std::vector<myDate> repricing_dates;    
    std::vector<double> par_nominals_begin;
    std::vector<double> par_nominals_end;
    bool opt_flg = false;
    double caplet_rate = 0.0;
    double caplet_vol = 0.0;
    double caplet_d = 0.0;
    double caplet_npv = 0.0;
    double floorlet_rate = 0.0;
    double floorlet_vol = 0.0;
    double floorlet_d = 0.0;
    double floorlet_npv = 0.0;
};

// bond data
struct cap_flr_info
{
    std::string ent_nm;
	std::string parent_id;
	std::string contract_id;
	std::string ptf;
    std::string issuer_id;
    std::string account;
	std::string isin;
    std::string comments;
    std::string cap_floor_type;
	bool is_fixed = true;
	std::string fix_type;
	std::string rtg;
	std::string ccy_nm;
	double nominal = 0.0;
	myDate deal_date;
	myDate maturity_date;
    std::string dcm;
    bool is_acc_int = false;
    double acc_int = 0.0;
    double acc_int_ref_ccy = 0.0;
	double cpn_rate = 0.0;
    double cap_rate = 0.0;
    std::string cap_vol_surf;
    double floor_rate = 0.0;
    std::string floor_vol_surf;
	myDate first_cpn_date;
	std::string cpn_freq;
	myDate first_fix_date;
	std::string fix_freq;
	myDate first_amort_date;
	std::string amort_freq;
	double amort = 0.0;
	std::string crv_disc;
	std::string crv_fwd;
    double npv = 0.0;
    double npv_ref_ccy = 0.0;
    std::string wrn_msg = "";
    std::vector<cap_flr_event> events;
};

/*
 * CAP / FLOOR CLASS
 */

class myCapsFloors
{
    private:
        // variables
        std::vector<cap_flr_info> info;

    public:
        // object constructors
        myCapsFloors(std::vector<cap_flr_info> info){this->info = info;};
        myCapsFloors(const mySQLite &db, const std::string &sql, const myDate &calc_date);

        // copy constructor
        myCapsFloors(const myCapsFloors &caps_flrs){this->info = caps_flrs.info;};

        // object destructors
        ~myCapsFloors(){};

        // object function declarations
        void clear(){this->info.clear();};
        std::vector<myCapsFloors> split(const int &threads_no);
        void merge(std::vector<myCapsFloors> &caps_flrs);
        void calc_npv(const int &scn_no, const myCurves &crvs, const myFx &fx, const std::string &ref_ccy_nm);
        std::thread calc_npv_thrd(const int &scn_no, const myCurves &crvs, const myFx &fx, const std::string &ref_ccy_nm);
        void write_npv(mySQLite &db, const int &scn_no, const std::string &ent_nm, const std::string &ptf);
};
