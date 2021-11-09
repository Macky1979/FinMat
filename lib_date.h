#pragma once

#include <iostream>
#include <string>
#include <vector>

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
        myDate();
        myDate(const string &, const string &);
        myDate(const int &);

        // copy constructor
        myDate (const myDate &_date);

        // object destructor
        ~myDate(){};

        // overloaded operators
        long operator- (const myDate &date2);
        myDate& operator= (const myDate &_date);

        // object function declarations
        void recalc();
        int get_year() const {return year;}
        int get_month() const {return month;}
        int get_day() const {return day;}
        int get_days_no() const {return days_no;}
        int get_date_int() const {return date_int;}
        string get_date_str() const {return date_str;}
        void add(const string &date_freq);
};

// external functions
std::vector<myDate> * create_date_serie(const string &date_str_begin, const string &date_str_end, const string &date_freq, const string &date_format);