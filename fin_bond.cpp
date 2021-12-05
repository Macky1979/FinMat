#include <string>
#include <iostream>
#include <tuple>
#include <map>
#include <vector>
#include "lib_date.h"
#include "fin_curve.h"
#include "fin_fx.h"
#include "fin_bond.h"

using namespace std;

/*
 * OBJECT CONSTRUCTORS
 */

myBond::myBond(const mySQLite &db, const string &sql)
{
    // dataframe to hold result of SQL query
    myDataFrame * rslt = new myDataFrame();

    // load bond portfolio as specified by SQL query
    rslt = db.query(sql);

    // go bond by bond
    for (int bnd_idx = 0; bnd_idx < rslt->tbl.values.size(); bnd_idx++)
    {
        // load bond information and perform some sanity checks
        bnd_info bnd;
        string aux;

            // entity name
            bnd.ent_nm = rslt->tbl.values[bnd_idx][0];

            // parent id; useful to bind two or more bonds together to create a new product like IRS
            bnd.parent_id = rslt->tbl.values[bnd_idx][1];

            // contract id
            bnd.contract_id = rslt->tbl.values[bnd_idx][2];

            // portfolio
            bnd.ptf = rslt->tbl.values[bnd_idx][3];

            // account
            bnd.account = rslt->tbl.values[bnd_idx][4];

            // bond ISIN
            bnd.isin = rslt->tbl.values[bnd_idx][5];

            // comments
            bnd.comments = rslt->tbl.values[bnd_idx][6];

            // bond type, e.g. PAM, RGM, ZCB
            bnd.bnd_type = rslt->tbl.values[bnd_idx][7];

            // field indicating if the bond is fixed or floating
            bnd.is_fixed = rslt->tbl.values[bnd_idx][8].compare("1") == 0 ? true : false;

            // fixing type (forward rate vs. par-rate) for a floating bond
            bnd.fix_type = rslt->tbl.values[bnd_idx][9];

            // bond rating
            bnd.rtg = rslt->tbl.values[bnd_idx][10];

            // bond currency, e.g. EUR, CZK
            bnd.ccy_nm = rslt->tbl.values[bnd_idx][11];

            // bond nominal
            bnd.nominal = stod(rslt->tbl.values[bnd_idx][12]);

            // bond deal date, i.e. date when the bond entered / will enter into books
            bnd.deal_date = myDate(stoi(rslt->tbl.values[bnd_idx][13]));

            // bond maturity date
            bnd.maturity_date = myDate(stoi(rslt->tbl.values[bnd_idx][14]));

            // bond accrued interest
            aux = rslt->tbl.values[bnd_idx][15];
            if (aux.compare("") == 0) // accrued interest was not provided
            {
                bnd.wrn_msg += "accrued interest not specified;";
                bnd.is_acc_int = false;
                bnd.acc_int = 0.0;
            }
            else // accrued interest was provided
            {
                bnd.is_acc_int = true;
                bnd.acc_int = stod(aux);
            }

            // coupon rate
            aux = rslt->tbl.values[bnd_idx][16];
            if (aux.compare("") == 0) // coupon rate was not provided
            {
                bnd.wrn_msg += "coupon rate not specified;";
                bnd.is_acc_int = false;
                bnd.acc_int = 0.0;
                bnd.cpn_rate = 0.0;
                bnd.first_cpn_date = bnd.maturity_date;
            }
            else // coupon rate was provided
            {
                bnd.cpn_rate = stod(aux);
            }
             
            // first coupon date
            aux = rslt->tbl.values[bnd_idx][17];
            if (aux.compare("") == 0) // first coupon date was not provided
            {
                bnd.wrn_msg += "first coupon date not specified;";
                bnd.is_acc_int = false;
                bnd.acc_int = 0.0;
                bnd.cpn_rate = 0.0;
                bnd.cpn_freq = "";
                bnd.first_cpn_date = bnd.maturity_date;
            }
            else // the first coupon date was provided
            {
                bnd.first_cpn_date = myDate(stoi(aux));
            }

            // coupon frequency
            bnd.cpn_freq = rslt->tbl.values[bnd_idx][18];
            
            // first fixing date of a floating bond
            aux = rslt->tbl.values[bnd_idx][19];
            if (aux.compare("") == 0) // fixing date not provided
            {
                if (!bnd.is_fixed) // floating bond
                {
                    bnd.wrn_msg += "first fixing date not provided for a floating bond;";
                    bnd.is_fixed = true;
                }
            }
            else // fixing date provided
            {
                if (!bnd.is_fixed) // floating bond
                {
                    bnd.first_fix_date = myDate(stoi(aux));
                }
                else // fixed bond
                {
                    bnd.wrn_msg += "first fixing date provided for a fixed bond;";
                }
            }
            
            // fixing frequency
            aux = rslt->tbl.values[bnd_idx][20];
            if (aux.compare("") == 0) // fixing frequency not specified
            {
                if (!bnd.is_fixed) // floating bond
                {
                    bnd.wrn_msg += "fixing frequency not provide a floating bond";
                    bnd.is_fixed = true;
                }
            }
            else // fixing frequency provided
            {
                if (!bnd.is_fixed) // floating bond
                {
                    bnd.fix_freq = aux;
                }
                else // fixed bond
                {
                    bnd.wrn_msg += "fixing frequency provide for a fixed bond;";
                }
            }

            // amortization
            string first_amort_date = rslt->tbl.values[bnd_idx][21];
            string amort_freq = rslt->tbl.values[bnd_idx][22];
            string amort = rslt->tbl.values[bnd_idx][23];
            
            if (amort.compare("") == 0) // amortization amount is not provided
            {
                bnd.amort = 0.0;

                if (amort_freq.compare("") != 0) // amortization frequency provided
                {
                    bnd.wrn_msg += "amortization frequency provided for a bond with zero amortization amount;";
                }
                bnd.amort_freq = "";

                if (first_amort_date.compare("") != 0) // amortization frequency provided 
                {
                    bnd.wrn_msg += "first amortization date provided for a bond with zero amortization amount;";
                }
            }
            else // amortization amount provided
            {
                bnd.amort = stod(amort);

                if (amort_freq.compare("") == 0) // amortization frequency not provided
                {
                    bnd.wrn_msg += "amortization frequency not provided for a bond with non-zero amortization amount;";
                    bnd.amort = 0.0;
                    bnd.amort_freq = "";
                }
                else // amortization frequency provided
                {
                    bnd.amort_freq = amort_freq;
                }

                if (first_amort_date.compare("") == 0) // first amortization date not provided
                {
                    bnd.wrn_msg += "first amortization date not provided for a bond with non-zero amortization amount;";
                    bnd.amort = 0.0;
                    bnd.amort_freq = "";
                }
                else // first amortization date provided
                {
                    bnd.first_amort_date = myDate(stoi(first_amort_date));
                }
            }
            
            // multiplier applied on bond repricing rate
            aux = rslt->tbl.values[bnd_idx][24];
            if (aux.compare("") == 0) // rate multiplier not specified
            {
                if (!bnd.is_fixed) // floating bond
                {
                    bnd.wrn_msg += "rate multiplier not specified for a floating bond;";
                    bnd.rate_mult = 1.0;
                }
                
            }
            else // rate multiplied specified
            {
                if (!bnd.is_fixed) // floating bond
                {
                    bnd.rate_mult = stod(aux);
                }
                else // fixed bond
                {
                    bnd.wrn_msg += "rate multiplier specified for a fixed bond;";
                }
            }

            // spread to be added to a repricing rate
            aux = rslt->tbl.values[bnd_idx][25];
            if (aux.compare("") == 0) // spread not specified
            {
                if (!bnd.is_fixed) // floating bond
                {
                    bnd.wrn_msg += "repricing spread not specified for a flating bond;";
                    bnd.rate_add = 0.0;
                }
            }
            else // spread specified
            {
                if (!bnd.is_fixed) // floating bond
                {
                    bnd.rate_add = stod(aux);
                }
                else // fixed bond
                {
                    bnd.wrn_msg += "repricing spread specified for a fixed bond;";
                }
            }
            
            // discounting curve
            bnd.curve_disc = rslt->tbl.values[bnd_idx][26];

            // repricing curve
            aux = rslt->tbl.values[bnd_idx][27];
            if (aux.compare("") == 0) // repricing curve not specified
            {
                if (!bnd.is_fixed) // floating bond
                {
                    bnd.wrn_msg += "repricing curve not specified for a floating bond;";
                    bnd.is_fixed = true;
                    bnd.fix_freq = "";
                }
            }
            else // repricing curve specified
            {
                if (!bnd.is_fixed) // floating bond
                {
                    bnd.curve_fwd = aux;
                }
                else // fixed bond
                {
                    bnd.wrn_msg += "repricing curve specified for a fixed bond;";
                    bnd.curve_fwd = "";
                }
            }

            // reset NPV
            bnd.npv = 0.0;

        // perform other sanity checks
    }
}

/*
 * OBJECT FUNCTIONS
 */


