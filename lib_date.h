#pragma once

#include <string>
#include <iostream>
#include "lib_date.cpp"

using namespace std;

// external functions
int days_in_month(const int &year, const int &month);
int * add_months(const int &year, const int &month, const int &day, const int &months_to_add);

// define date class
class myDate 
{
    // private variables
    string date_str;
    int date_int;
    int year;
    int month;
    int day;
    long days_no;
    const int init_year = 1601;

    // private functions
    void set_year_month_day();
    void set_days_no();

    public:
        // object constructors
        myDate(const string &, const string &);
        myDate(const int &);

        // object destructor
        ~myDate(){};

        // overloaded operators
        long operator - (const myDate&);

        // function declarations
        void recalc();
        int get_year(){return year;}
        int get_month(){return month;}
        int get_day(){return day;}
        int get_days_no(){return days_no;}
        int get_date_int(){return date_int;}
        string get_date_str(){return date_str;}
        void add(const string &date_freq);
};

void myDate::set_year_month_day()
{
    // determine year, month and day
    year = stoi(date_str.substr(0, 4));
    month = stoi(date_str.substr(4, 2));
    day = stoi(date_str.substr(6,2));
}

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

void myDate::recalc()
{
    // convert yyyymmdd date string into integer
    date_int = stoi(date_str);

    // set year, month and day
    set_year_month_day();

    // set days_no
    set_days_no();
}

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
    recalc();

}

myDate::myDate(const int &_date_int)
{
    // store date integer; we assume yyyymmdd format
    date_int = _date_int;

    // store date as string in yyyymmdd
    date_str = to_string(_date_int);

    // determine year, month, day and number of days since 01/01/1601
    recalc();

}

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

long myDate::operator- (const myDate &date2)
{
    return days_no - date2.days_no;
}