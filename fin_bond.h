# pragma once

#include <iostream>
#include <string>
#include <vector>
#include "lib_date.h"
#include "fin_curve.h"

// coupon data type
struct coupon
{
    bool appl = false;
    bool fixed = false;
    std::string date1;
    std::string date2;
    float year_frac;
    float rate;
};

// event data type
struct event
{
    int date;
    float nominal;
    bool is_cpn_payment = false;
    bool is_repricing = false;
    bool is_amort = false;
    bool is_cpn_fix = false;
    float cpn;
    std::vector<myDate> repricing_dates;    
    float repricing_year_frac;
    std::vector<myDate> cpn_dates;
    float cpn_year_frac;
    float cpn_payment;
    float amort_payment;
    float cf;
};

// bond data
struct bnd_info
{
    std::string ent_nm;
	std::string parent_id;
	std::string contract_id;
	std::string ptf;
    std::string account;
	std::string isin;
    std::string comments;
	std::string bnd_type;
	bool is_fixed;
	std::string fix_type;
	std::string rtg;
	std::string ccy_nm;
	float nominal;
	myDate deal_date;
	myDate maturity_date;
	bool is_acc_int;
    float acc_int;
	float cpn_rate;
	myDate first_cpn_date;
	std::string cpn_freq;
	myDate first_fix_date;
	std::string fix_freq;
	myDate first_amort_date;
	std::string amort_freq;
	double amort;
	double rate_mult;
	double rate_add;
	std::string curve_disc;
	std::string curve_fwd;
    float npv;
    std::string wrn_msg = "";
    std::vector<event> events;
};

/*
 * BOND CLASS
 */

class myBond
{
    private:
        // variables
        bnd_info info;

    public:
        // object constructors
        myBond(const mySQLite &db, const std::string &sql);

        // object destructors
        ~myBond(){};

        // object function declarations
        //void myBond::check();
        //void myBond::init(const myDate &calc_date);
        //void myBond::calc_npv(const int &scn_no, std::vector<myCurve>, std::vector<myFx>);
};
