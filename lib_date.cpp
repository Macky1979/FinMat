#include <string>
#include <iostream>
#include <vector>
#include "lib_date.h"

using namespace std;

/*
 * GENERAL FUNCTIONS
 */

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

// create a vector of dates from start date to end date using time step of a given frequency
vector<myDate> * create_date_serie(const string &date_str_begin, const string &date_str_end, const string &date_freq, const string &date_format)
{
    // create vector to hold date serie
    vector<myDate> * date_serie = new vector<myDate>();

    //create date end and current date objects
    myDate date_end(date_str_end, date_format);
    myDate date_current(date_str_begin, date_format);

    // create date series
    while (date_current.get_date_int() < date_end.get_date_int())
    {
        (*date_serie).push_back(date_current);
        date_current.add(date_freq);
    }

    // return pointer to the vector with date serie
    return date_serie;
}

/*
 * OBJECT FUNCTIONS
 */

// set year, month and day based on date string in yyyymmdd format
void myDate::set_year_month_day()
{
    // determine year, month and day
    year = stoi(date_str.substr(0, 4));
    month = stoi(date_str.substr(4, 2));
    day = stoi(date_str.substr(6,2));
}

// derive date "distance" in days from initial date of 01/01/1601
void myDate::set_days_no()
{
    // calculate number of days for initial date
        // years contribution
        days_no = (year - init_year) * 365;
        days_no += (year - init_year) / 4; // leap years
        days_no -= (year - init_year) / 100; // we skip leap years every 100 years
        days_no += (year - init_year) / 400; // unless the year is divisible by 400
        // months contribution
        for (int i = month - 1; i > 0; --i)
        {
            days_no += days_in_month(year, i);
        }
        // days contribution
        days_no += day - 1;
}

// update date integer, year, month and day based on date string in format yyyymmdd
void myDate::recalc()
{
    // convert yyyymmdd date string into integer
    date_int = stoi(date_str);

    // set year, month and day
    this->set_year_month_day();

    // set days_no
    this->set_days_no();
}

//shift date forward using specified date frequency (e.g. 2D, 3M, 10Y)
void myDate::add(const string &date_freq)
{
    // variables to hold frequency type (e.g. D, M, Y) and number of frequency units
    int date_freq_no;
    string date_freq_type;

    // determine frequency units
    date_freq_no = stoi(date_freq.substr(0, date_freq.size() - 1));

    // determine frequency type
    date_freq_type = date_freq.substr(date_freq.size() - 1, 1);

    if (date_freq_type.compare("D") == 0)
    {
        int _days_no;
        int _years_to_add;
        int _months_to_add;
        int _days_in_month;
        string _freq_date;

        // store number of days in the current date
        _days_no = days_no;

        // add years to the current dayes
        _years_to_add = date_freq_no / 366;
        if (_years_to_add != 0)
        {
            _days_no = this->get_days_no();
            _freq_date = to_string(_years_to_add) + "Y";
            this->add(_freq_date);
            date_freq_no -= days_no - _days_no;
        }

        // add months in bulk
        _months_to_add = date_freq_no / 31;
        if (_months_to_add != 0)
        {
            _days_no = this->get_days_no();
            _freq_date = to_string(_months_to_add) + "M";
            this->add(_freq_date);
            date_freq_no -= days_no - _days_no;
        }

        // add the last one or two months
        _days_in_month = days_in_month(year, month);
        while (date_freq_no + day > _days_in_month)
        {
            _days_no = this->get_days_no();
            _freq_date = "1M";
            this->add(_freq_date);
            date_freq_no -= days_no - _days_no;
            _days_in_month = days_in_month(year, month);
        }
        
        // add remaining days
        day += date_freq_no;
        date_str = to_string(year * 10000 + month * 100 + day);
        recalc();

    }
    else if (date_freq_type.compare("M") == 0)
    {
        int * date;
        date = add_months(year, month, day, date_freq_no);
        year = date[0];
        month = date[1];
        day = date[2];
        date_str = to_string(year * 10000 + month * 100 + day);
        recalc();
    }
    else if (date_freq_type.compare("Y") == 0)
    {
        year += date_freq_no;
        date_str = to_string(year * 10000 + month * 100 + day);
        recalc();
    }
    else
    {
        throw std::invalid_argument(date_freq_type + " is  notsupported date date frequency type!" );
    }
}

/*
 * OBJECT CONSTRUCTORS
 */

myDate::myDate()
{
    date_str = "16010101";
    date_int = 16010101;
    year = 1601;
    month = 1;
    day = 1;
    days_no = 0;
};

myDate::myDate(const string &_date_str, const string &_date_format)
{
    // convert date string into yyyymmdd string format
    if (_date_format.compare("dd/mm/yyyy") == 0)
    {
        date_str = _date_str.substr(6,4) + _date_str.substr(3,2) + _date_str.substr(0, 2);
    }
    else if (_date_format.compare("yyyy/mm/dd") == 0)
    {
        date_str = _date_str.substr(0,4) + _date_str.substr(5,2) + _date_str.substr(8, 2);
    }
    else if (_date_format.compare("ddmmyyyy") == 0)
    {
        date_str = _date_str.substr(4, 4) + _date_str.substr(2, 2) + _date_str.substr(0, 2);
    }
    else if (_date_format.compare("yyyymmdd") == 0)
    {
        date_str = _date_str;
    }
    else
    {
        throw std::invalid_argument(_date_format + " is  notsupported date string format!" );
    }

    // determine year, month, day and number of days since 01/01/1601
    this->recalc();

}

myDate::myDate(const int &_date_int)
{
    // store date integer; we assume yyyymmdd format
    date_int = _date_int;

    // store date as string in yyyymmdd
    date_str = to_string(_date_int);

    // determine year, month, day and number of days since 01/01/1601
    this->recalc();

}

/*
 * COPY CONSTRUCTOR
 */

myDate::myDate (const myDate &_date)
{
    date_str = _date.get_date_str();
    this->recalc();
};

/*
 * OBJECT OPERATORS
 */

long myDate::operator- (const myDate &date2)
{
    return days_no - date2.get_days_no();
}

myDate& myDate::operator= (const myDate &_date)
{
    date_str = _date.get_date_str();
    this->recalc();
    return *this;
};

