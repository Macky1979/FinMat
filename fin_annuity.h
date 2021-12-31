# pragma once

#include <iostream>
#include <string>
#include <vector>
#include "lib_date.h"
#include "fin_curve.h"
#include "fin_fx.h"

// event data type
struct event
{
    myDate date_begin;
    myDate date_end;
    int rmng_ann_payments = 0;
    bool is_ann_fixed = false;
    bool fix_flg = false;
    double nominal_begin = 0.0;
    double int_rate = 0.0;
    double rate_mult = 0.0;
    double rate_add = 0.0;
    double ext_rate = 0.0;
    double ext_ann_payment = 0.0;
    double ext_int_payment = 0.0;
    double int_int_payment = 0.0;
    double ext_amort_payment = 0.0;
    double nominal_end = 0.0;
    std::vector<myDate> repricing_dates;    
    std::vector<double> par_nominals_begin;
    std::vector<double> par_nominals_end;
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
    double npv = 0.0;
    double npv_ref_ccy = 0.0;
    std::string wrn_msg = "";
    std::vector<event> events;
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
        myAnnuities(const mySQLite &db, const std::string &sql, const myDate &calc_date);

        // object destructors
        ~myAnnuities(){};

        // object function declarations
        void calc_npv(const int &scn_no, const myCurves &crvs, const myFx &fx, const std::string &ref_ccy_nm);
        void write_npv(mySQLite &db, const int &scn_no, const std::string &ent_nm, const std::string &ptf);
};
