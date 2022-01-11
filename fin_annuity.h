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
#include "fin_annuity.h"

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
    std::string bnd_data = "data/ann_data.csv";
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

    sql = read_sql(sql_file_nm, "ann_data");
    db.exec(sql);

    sql = read_sql(sql_file_nm, "ann_npv");
    db.exec(sql);

    // delete old content in the tables
    db.exec("DELETE FROM cnty_def;");
    db.exec("DELETE FROM ccy_def;");
    db.exec("DELETE FROM ccy_data;");
    db.exec("DELETE FROM freq_def;");
    db.exec("DELETE FROM dcm_def;");
    db.exec("DELETE FROM crv_def;");
    db.exec("DELETE FROM crv_data;");
    db.exec("DELETE FROM ann_data;");
    db.exec("DELETE FROM ann_npv;");

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
    db.upload_tbl(*rslt, "ann_data", delete_old_data);
    rslt->clear();

    // vacuum SQLite database file to avoid its excessive growth
    db.vacuum();

    std::cout << get_timestamp() + " - initiating curves and FX rates..." << std::endl;

    // load FX rates
    myFx fx = myFx(db, sql_file_nm);

    // load all curves
    myCurves crvs = myCurves(db, sql_file_nm, calc_date);

    std::cout << get_timestamp() + " - initiating annuities..." << std::endl;

    // define entity and portfolio
    std::string ent_nm = "kbc";
    std::string ptf = "ann";

    // load annuities
    myAnnuities anns = myAnnuities(db, "SELECT * FROM ann_data WHERE ent_nm = '" + ent_nm + "' AND ptf = '" + ptf + "';", calc_date);
    
    // define scenario number and reference currency to be used in valuation
    int scn_no = 1;
    std::string ref_ccy_nm = "EUR";

    std::cout << get_timestamp() + " - evaluating annuities on a single core..." << std::endl;

    // evaluate annuities using single core
    anns.calc_npv(scn_no, crvs, fx, ref_ccy_nm);

    std::cout << get_timestamp() + " - evaluating annuities using multithreading..." << std::endl;

    // evaluate annuties using multiple cores
    int threads_no = 4;

    std::cout << get_timestamp() + " -    spliting contracts..." << std::endl;

    std::vector<myAnnuities> anns_thrd = anns.split(threads_no);

    std::cout << get_timestamp() + " -    running individual threads..." << std::endl;

    std::vector<std::thread> workers;
    for (int thread_idx = 0; thread_idx < threads_no; thread_idx++)
    {
        workers.emplace_back(anns_thrd[thread_idx].calc_npv_thrd(scn_no, crvs, fx, ref_ccy_nm));
    }

    std::cout << get_timestamp() + " -    waiting for individual threads to finish..." << std::endl;

    for (int thread_idx = 0; thread_idx < threads_no; thread_idx++)
    {
        workers[thread_idx].join();
    }

    std::cout << get_timestamp() + " -    merging results..." << std::endl;

    // merge results into a single vector
    anns.merge(anns_thrd);

    std::cout << get_timestamp() + " - storing NPV into SQLite database file..." << std::endl;

    // store results
    anns.write_npv(db, scn_no, ent_nm, ptf);

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
struct ann_event
{
    myDate date_begin;
    myDate date_end;
    int rmng_ann_payments = 0;
    bool is_ann_fixed = false;
    bool fix_flg = false;
    double nominal_begin = 0.0;
    double int_rate = 0.0;
    double ext_rate = 0.0;
    double ext_ann_payment = 0.0;
    double ext_int_payment = 0.0;
    double int_int_payment = 0.0;
    double ext_amort_payment = 0.0;
    double nominal_end = 0.0;
    std::vector<myDate> repricing_dates;
    double ext_cf = 0.0;
    double int_cf = 0.0;
    double df = 0.0;
};

// annuity data
struct ann_info
{
    std::string ent_nm;
	std::string parent_id;
	std::string contract_id;
	std::string ptf;
    std::string issuer_id;
    std::string account;
	std::string isin;
    std::string comments;
	std::string ann_type;
	bool is_fixed = true;
	std::string fix_type;
	std::string rtg;
	std::string ccy_nm;
	double nominal = 0.0;
	myDate deal_date;
	myDate maturity_date;
    bool is_acc_int = true;
    double ext_acc_int = 0.0;
    double ext_acc_int_ref_ccy = 0.0;
	double int_rate = 0.0;
	double rate_mult = 0.0;
    double rate_add = 0.0;
    myDate first_ann_date;
	std::string ann_freq;
	double ann_freq_aux = 0.0;
    myDate first_fix_date;
	std::string fix_freq;
	std::string crv_disc;
	std::string crv_fwd;
    double int_npv = 0.0;
    double int_npv_ref_ccy = 0.0;
    double ext_npv = 0.0;
    double ext_npv_ref_ccy = 0.0;
    std::string wrn_msg = "";
    std::vector<ann_event> events;
};

/*
 * ANNUITY CLASS
 */

class myAnnuities
{
    private:
        // variables
        std::vector<ann_info> info;

    public:
        // object constructors
        myAnnuities(std::vector<ann_info> info){this->info = info;};
        myAnnuities(const mySQLite &db, const std::string &sql, const myDate &calc_date);

        // copy constructor
        myAnnuities(const myAnnuities &anns){this->info = anns.info;};

        // object destructors
        ~myAnnuities(){};

        // object function declarations
        void clear(){this->info.clear();};
        std::vector<myAnnuities> split(const int &threads_no);
        void merge(std::vector<myAnnuities> &anns);
        void calc_npv(const int &scn_no, const myCurves &crvs, const myFx &fx, const std::string &ref_ccy_nm);
        std::thread calc_npv_thrd(const int &scn_no, const myCurves &crvs, const myFx &fx, const std::string &ref_ccy_nm);
        void write_npv(mySQLite &db, const int &scn_no, const std::string &ent_nm, const std::string &ptf);
};
