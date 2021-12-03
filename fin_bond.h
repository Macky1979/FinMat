# pragma once

#include <iostream>
#include <string>
#include <vector>
#include "lib_date.h"
#include "fin_curve.h"

// bond data
struct bnd_info
{
    std::string entity;
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
    float deal_time;
	myDate maturity_date;
    float maturity_time;
	float acc_int;
	float cpn_rate;
	myDate first_cpn_date;
    float first_cpn_time;
	std::string cpn_freq;
	myDate first_fixing_date;
    float first_fixing_time;
	std::string fix_freq;
	myDate first_amort_date;
    float first_amort_time;
	std::string amort_freq;
	double amort;
	double rate_mult;
	double rate_add;
	std::string curve_disc;
	std::string curve_fwd;
    float npv;
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
        myBond(const bnd_info &_info)
        {
            info = _info;
        }

        // object destructors
        ~myBond(){};

        // object function declarations
        void myBond::check();
        void myBond::calc_times(const myDate &calc_date);
        void myBond::calc_npv(const int &scn_no, std::vector<myCurve>);
};