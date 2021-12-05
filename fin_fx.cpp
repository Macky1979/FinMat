#include <string>
#include <map>
#include <tuple>
#include <vector>
#include "lib_sqlite.h"
#include "fin_fx.h"

using namespace std;

/*
 * OBJECT CONSTRUCTORS
 */

myFx::myFx(const mySQLite &db, const string &sql_file_nm)
{
    // dataframe to hold result of SQL query
    myDataFrame * rslt = new myDataFrame();

    // variable to hold SQL query
    string sql;

    // load curve definition
    sql = read_sql(sql_file_nm, 3);
    rslt = db.query(sql);

    // go line by line and create map
    for (int idx = 0; idx < rslt->tbl.values.size(); idx++)
    {
        // create a tuple of scenario and currency name
        string ccy_nm = rslt->tbl.values[idx][0];
        int scn_no = stoi(rslt->tbl.values[idx][1]);
        tuple<int, string> ccy = {scn_no, ccy_nm};

        // add the data to FX object
        float rate = stod(rslt->tbl.values[idx][2]);
        this->data.insert(pair<tuple<int, string>, float>(ccy, rate));
    }

    // delete unused points
    delete rslt;
}

// get FX rate based on scenario number and currency name
float * myFx::get_fx(const tuple<int, string> &ccy)
{
    // get FX rate
    float * rate = &(this->data.at(ccy));

    // return FX rate
    return rate;
}
