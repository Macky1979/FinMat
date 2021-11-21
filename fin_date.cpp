
#include <string>
#include <algorithm>
#include "fin_date.h"

using namespace std;

// calculate year fractions for family of 30/360 methods
float calc_year_fraction(const int &day1, const int &month1, const int &year1, const int &day2, const int &month2, const int &year2)
{
        // calculate year fraction
        return (360. * (year2 - year1) + 30. * (month2 - month1) + (day2 - day1)) / 360.;
}

// implementation of day count method; see https://en.wikipedia.org/wiki/Day_count_convention
float day_count_method(const myDate &date1, const myDate &date2, const string &dcm)
{
    // variables
    int day1;
    int day2;
    int month1;
    int month2;
    int year1;
    int year2;
    float year_fraction;
    bool last_february_day1;
    bool last_february_day2;

    using namespace std;

    // check that date1 preceeds date2
    if (date1.get_days_no() >= date2.get_days_no())
    {
        throw std::invalid_argument("Parameter date1 must preceed date2!");
    }

    // extract day, month and year from date 1
    day1 = date1.get_day();
    month1 = date1.get_month();
    year1 = date1.get_year();

    // extract day, month and year from date 2
    day2 = date2.get_day();
    month2 = date2.get_month();
    year2 = date2.get_year();

    // 30/360 bond basis
    if (dcm.compare("30_360") == 0)
    {
        // adjust day1
        day1 = min(day1, 30);
    
        // adjust day2
        if (day1 > 29)
        {
            day2 = min(day2, 30);
        }

        // calculate year fraction
        year_fraction = calc_year_fraction(day1, month1, year1, day2, month2, year2);
    }
    // 30/360 US
    else if (dcm.compare("30US_360") == 0)
    {

        // determine if date1 is the last February day
        if ((month1 == 2) and date1.is_last_day_in_month())
        {
            last_february_day1 = true;
        }
        else
        {
            last_february_day1 = false;
        }

        // determine if date2 is the last February day
        if ((month2 == 2) and date2.is_last_day_in_month())
        {
            last_february_day2 = true;
        }
        else
        {
            last_february_day2 = false;
        }

        // adjust day2 if both date1 and date2 are the last February days
        if (last_february_day1 && last_february_day2)
        {
            day2 = 30;
        }

        // adjust day1 if date1 is the last February day
        if (last_february_day1)
        {
            day1 = 30;
        }

        // adjust day2
        if (day2 == 31 && ((day1 == 30) || (day1 == 31)))
        {
            day2 = 30;
        }

        // adjust day1
        if (day1 == 31)
        {
            day1 = 30;
        }

        // calculate year fraction
        year_fraction = calc_year_fraction(day1, month1, year1, day2, month2, year2);
    }
    // actual / 360 method
    else if (dcm.compare("ACT_360") == 0)
    {
        // calculate year fraction
        year_fraction = (date2.get_days_no() - date1.get_days_no()) / 360.;
    }
    // actual / 365 method
    else if (dcm.compare("ACT_365") == 0)
    {
        // calculate year fraction
        year_fraction = (date2.get_days_no() - date1.get_days_no()) / 365.;
    }
    // actual / actual ISDA method
    else if (dcm.compare("ACT_ACT") == 0)
    {
        // date1 and date2 fall into the same year
        if (year1 == year2)
        {
            // the year is a leap year
            if (date1.is_leap_year())
            {
                // calculate year fraction
                year_fraction = (date2.get_days_no() - date1.get_days_no()) / 366.;           
            }
            else
            {
                // calculate year fraction
                year_fraction = (date2.get_days_no() - date1.get_days_no()) / 365.; 
            }
        }
        // date1 and date2 fall into different years
        else
        {   
            // auxiliary variables
            float year_fraction1_aux;
            float year_fraction2_aux;

            // create auxiliary dates
            myDate date1_aux = myDate(date1.get_year() * 10000 + 1231);
            myDate date2_aux = myDate(date2.get_year() * 10000 + 0101);

            // calculate the first auxiliary year fraction
            year_fraction1_aux = date1_aux.get_days_no() - date1.get_days_no();
            if (date1.is_leap_year())
            {
                year_fraction1_aux /= 366;
            }
            else
            {
                year_fraction1_aux /= 365;
            }

            // calculate the seconds auxiliary year fraction
            year_fraction2_aux = date2.get_days_no() - date2_aux.get_days_no();
            if (date2.is_leap_year())
            {
                year_fraction2_aux /= 366;
            }
            else
            {
                year_fraction2_aux /= 365;
            }

            // calculate the final year fraction
            year_fraction = year_fraction1_aux + year_fraction2_aux + (date2.get_year() - date1.get_year() - 1);
        }
    }
    // supported day count method
    else
    {
        throw std::invalid_argument(dcm + " is not a supported day count method!");
    }

    // return calcualted year fraction
    return year_fraction;
}