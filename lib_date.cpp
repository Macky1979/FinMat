#include <string>
#include <iostream>
#include "lib_date.h"

using namespace std;

// determine number of days in month
int days_in_month(const int &year, const int &month)
{
    switch (month)
    {
        // months that have 31 days
        case 1:
        case 3:
        case 5:
        case 7:
        case 8:
        case 10:
        case 12:
            return 31;
        // months that have 30 days
        case 4:
        case 6:
        case 9:
        case 11:
            return 30;
        // February
        case 2:
            if (year % 4 == 0)
                return 29;
            else
                return 28;
        // incorrect month
        default:
            throw std::invalid_argument(to_string(month) + "  is not supported month!" );
    }
}

// add months
int * add_months(const int &year, const int &month, const int &day, const int &months_to_add)
{
    // declare array of integers to hold information on year, month and day
    static int date[3];

    // individual date components
    int _years_to_add;
    int _year;
    int _month;
    int _day;

    // determine how many years and months should be added straight away
    _years_to_add = months_to_add / 12;
    _month = months_to_add % 12;

    // determine month
    if ((_month > 0) && (_month + month) > 12)
    {
        _years_to_add++;
        _month = (_month + month) % 12;
    }
    else
    {
        _month += month;
    }

    // determine year
    _year = year + _years_to_add;

    // check maximum day in month
    _day = days_in_month(_year, _month);
    if (_day > day)
        _day = day;


    // return result
    date[0] = _year;
    date[1] = _month;
    date[2] = _day;

    return date;
}