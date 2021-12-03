#pragma once

#include <iostream>
#include <string>
#include <vector>

/*
#include <string>
#include <iostream>
#include "lib_date.h"

using namespace std;

int main()
{

    // introduction of the myDate object
    string date_str = "30/12/1979";
    string date_format = "dd/mm/yyyy";
    myDate my_birth(date_str, date_format);
    cout << "I was born " + to_string(my_birth.get_days_no()) + " days after 01/01/1601." << endl;

    // overloaded operators
    myDate today(20211107);
    cout << today - my_birth << " days being alive!" << endl;
    myDate some_day;
    some_day = today;
    cout << "Today is " + some_day.get_date_str() << endl;

    // copy construtor and function add()
    myDate birthsday = my_birth;
    birthsday.add("40Y");
    cout << "I was born on " + my_birth.get_date_str() + "." << endl;
    cout << "I was 40Y on " + birthsday.get_date_str() + "." << endl;
    birthsday.add("22M");
    cout << "Today I am " + to_string((birthsday - my_birth) / 365.) + " years old." << endl;

    // pointer to myDate object
    myDate * christmas = new myDate(20211224);
    cout << "Next X-mas is on " + christmas->get_date_str() + "." << endl;
    delete christmas;

    // default constructor
    myDate renesaince;
    cout << "The lowest possible date is " + renesaince.get_date_str() + "." << endl;

    // all days in my life; demonstration of create_date_series() function
    string date_str_begin = "19791230";
    string date_str_end = "20211107";
    string date_freq = "1D";
    date_format = "yyyymmdd";
    vector<myDate> * my_life = create_date_serie(date_str_begin, date_str_end, date_freq, date_format);
    cout << "I am alive " + to_string(my_life->size()) + " days!" << endl;
    cout << "Do you want to list all the days? (y/n)";
    string answer;
    cin >> answer;
    if ((answer.compare("y") == 0) || (answer.compare("Y") == 0))
    {
        for (int i = 0; i <= my_life->size(); i++)
        {
            cout << (*my_life)[i].get_date_str() << '\n';
        }
    }
    delete my_life;

    // everything OK
    return 0;
}
*/

// define date class
class myDate
{
    private:
        // variables
        std::string date_str;
        int date_int;
        int year;
        int month;
        int day;
        long days_no;
        const int init_year = 1601;

        // functions
        void set_year_month_day();
        void set_days_no();

    public:
        // object constructors
        myDate();
        myDate(const std::string &date_str, const std::string &date_format);
        myDate(const int &date_int);

        // copy constructor
        myDate (const myDate &date);

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
        std::string get_date_str() const {return date_str;}
        void add(const std::string &date_freq);
        bool is_last_day_in_month() const;
        bool is_leap_year() const;
};

// external functions
std::vector<myDate> * create_date_serie(const std::string &date_str_begin, const std::string &date_str_end, const std::string &date_freq, const std::string &date_format);