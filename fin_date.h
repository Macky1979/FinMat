#pragma once

#include "lib_date.h"

/*
#include <string>
#include <iostream>
#include <vector>
#include <sqlite3.h>
#include "lib_dataframe.h"
#include "lib_sqlite.h"
#include "lib_aux.h"
#include "fin_date.h"

int main()
{
    // day count method
    std::string dcm;

    // year fraction variable
    double year_fraction;

    // create date objects
    myDate date1 = myDate("30/12/1979", "dd/mm/yyyy");
    myDate date2 = myDate("21/11/2021", "dd/mm/yyyy");

    // 30/360
    dcm = "30_360";
    year_fraction = day_count_method(date1, date2, dcm);
    std::cout << "Year fraction between dates " + date1.get_date_str() + " and " + date2.get_date_str() +\
        " assuming " + dcm + " is " + std::to_string(year_fraction) + " years" << std::endl;

    // actual/360
    dcm = "ACT_360";
    year_fraction = day_count_method(date1, date2, dcm);
    std::cout << "Year fraction between dates " + date1.get_date_str() + " and " + date2.get_date_str() +\
        " assuming " + dcm + " is " + std::to_string(year_fraction) + " years" << std::endl;

    // actual/365
    dcm = "ACT_365";
    year_fraction = day_count_method(date1, date2, dcm);
    std::cout << "Year fraction between dates " + date1.get_date_str() + " and " + date2.get_date_str() +\
        " assuming " + dcm + " is " + std::to_string(year_fraction) + " years" << std::endl;

    // actual/actual
    dcm = "ACT_ACT";
    year_fraction = day_count_method(date1, date2, dcm);
    std::cout << "Year fraction between dates " + date1.get_date_str() + " and " + date2.get_date_str() +\
        " assuming " + dcm + " is " + std::to_string(year_fraction) + " years" << std::endl;

    // everything OK
    return 0;
}
*/

double day_count_method(const myDate &date1, const myDate &date2, const std::string &dcm);
