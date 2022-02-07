#include <string>
#include <map>
#include <math.h>
#include <tuple> 
#include <vector>
#include "lib_sqlite.h"
#include "lib_lininterp.h"
#include "fin_date.h"
#include "fin_vol_surf.h"

/*
 * OBJECT CONSTRUCTORS
 */

// object containing information on a volatility surface
myVolSurface::myVolSurface(const mySQLite &db, const std::string &sql_file_nm, const std::string &vol_surf_nm)
{
    // dataframe to hold result of SQL query
    myDataFrame * rslt = new myDataFrame();

    // variable to hold SQL query
    std::string sql;

    // load volatility surface definition
    sql = read_sql(sql_file_nm, "load_vol_surf_def");
    sql = replace_in_sql(sql, "##vol_surf_nm##", "'" + vol_surf_nm + "'");
    rslt = db.query(sql);

    // initiate volatility surface definition variables
    this->vol_surf_nm = rslt->tbl.values[0][0];
    this->ccy_nm = rslt->tbl.values[0][1];
    this->vol_surf_type = rslt->tbl.values[0][2];
    this->underlying = rslt->tbl.values[0][3];

    // load volatility surface data
    sql = read_sql(sql_file_nm, "load_vol_surf_data");
    sql = replace_in_sql(sql, "##vol_surf_nm##", "'" + vol_surf_nm + "'");
    rslt = db.query(sql);

    // go scenario by scenario
    int scn_no = -1;
    bool is_new_scn = false;
    std::vector<double> maturities;
    std::vector<double> strikes;
    std::vector<double> volatilities;

    for (int idx = 0; idx < rslt->tbl.values.size(); idx++)
    {
        // check that you have encountered a new scenario or you are at the end of file
        if (((stoi(rslt->tbl.values[idx][0]) != scn_no) && (scn_no != -1)) || (idx == rslt->tbl.values.size() - 1))
        {
            // create a tuple of scenario and volatility surface
            vol_surf_def vol_surf_aux = {maturities, strikes, volatilities};
            std::tuple<int, vol_surf_def> vol_surf = {scn_no, vol_surf_aux};
            this->vol_surf.insert(std::pair<int, vol_surf_def>(scn_no, vol_surf_aux));

            // clear variables
            maturities.clear();
            strikes.clear();
            volatilities.clear();
        }
        // load data
        {
            scn_no = stoi(rslt->tbl.values[idx][0]);
            maturities.push_back(stod(rslt->tbl.values[idx][1]));
            strikes.push_back(stod(rslt->tbl.values[idx][2]));
            volatilities.push_back(stod(rslt->tbl.values[idx][3]));
        }
    }

    // delete unused pointers
    delete rslt;
}

// object containing information on all volatility surfaces
myVolSurfaces::myVolSurfaces(const mySQLite &db, const std::string &sql_file_nm)
{
    // dataframe to hold result of SQL query
    myDataFrame * rslt = new myDataFrame();

    // variable to hold SQL query
    std::string sql;

    // load list of curves
    sql = read_sql(sql_file_nm, "load_all_vol_surf_nms");
    rslt = db.query(sql);

    // load volatility surface by volatility surface
    std::string vol_surf_nm;
    for (int vol_surf_idx = 0; vol_surf_idx < rslt->tbl.values.size(); vol_surf_idx++)
    {
        vol_surf_nm = rslt->tbl.values[vol_surf_idx][0];
        myVolSurface vol_surf = myVolSurface(db, sql_file_nm, vol_surf_nm);
        this->vol_surf.insert(std::pair<std::string, myVolSurface>(vol_surf_nm, vol_surf));
    }

    // delete unused pointers
    delete rslt;
}

/*
 * OBJECT FUNCTIONS
 */

// get surface volatilities based on scenario number and vector of maturities and strikes
std::vector<double> myVolSurface::get_vols(const int &scn_no, const std::vector<double> &maturities, const std::vector<double> &strikes) const
{
    // initiate 2D interpolation object
    vol_surf_def vol_surf_aux = this->vol_surf.at(scn_no);
    myLinInterp2D interp2D(vol_surf_aux.maturities, vol_surf_aux.strikes, vol_surf_aux.volatilities);

    // interpolate volatilities
    std::vector<double> volatilities = interp2D.eval(maturities, strikes);

    // return interpolated volatilities
    return volatilities;
}

// get year fraction based on vector of scenario numbers and tenor integer dates in yyyymmdd format
std::vector<double> myVolSurfaces::get_vols(const std::string &vol_surf_nm, const int &scn_no, const std::vector<double> &maturities, const std::vector<double> &strikes) const
{
    return this->vol_surf.at(vol_surf_nm).get_vols(scn_no, maturities, strikes);
}