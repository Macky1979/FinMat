#pragma once

#include <iostream>
#include <string>
#include <tuple>
#include <vector>

/*
#include <string>
#include <iostream>
#include "lib_date.h"

int main()
{

    // introduction of the myDate object
    std::string date_str = "30/12/1979";
    std::string date_format = "dd/mm/yyyy";
    myDate my_birth(date_str, date_format);
    std::cout << "I was born " + std::to_string(my_birth.get_days_no()) + " days after 01/01/1601." << std::endl;

    // overloaded operators
    myDate today(20211107);
    std::cout << today - my_birth << " days being alive!" << std::endl;
    myDate some_day;
    some_day = today;
    std::cout << "Today is " + some_day.get_date_str() << std::endl;

    // copy construtor and function add()
    myDate birthsday = my_birth;
    birthsday.add("40Y");
    std::cout << "I was born on " + my_birth.get_date_str() + "." << std::endl;
    std::cout << "I was 40Y on " + birthsday.get_date_str() + "." << std::endl;
    birthsday.add("22M");
    std::cout << "Today I am " + std::to_string((birthsday - my_birth) / 365.) + " years old." << std::endl;

    // function remove
    myDate yesterday = myDate(20211106);
    yesterday.remove("253M");
    std::cout << "253 months before 20211106 was " << yesterday.get_date_str() << std::endl;

    // pointer to myDate object
    myDate * christmas = new myDate(20211224);
    std::cout << "Next X-mas is on " + christmas->get_date_str() + "." << std::endl;
    delete christmas;

    // default constructor
    myDate renesaince;
    std::cout << "The lowest possible date is " + renesaince.get_date_str() + "." << std::endl;

    // all days in my life; demonstration of create_date_series() function
    std::string date_str_begin = "19791230";
    std::string date_str_end = "20211107";
    std::string date_freq = "1D";
    date_format = "yyyymmdd";
    std::vector<myDate> my_life = create_date_serie(date_str_begin, date_str_end, date_freq, date_format);
    std::cout << "I am alive " + std::to_string(my_life.size()) + " days!" << std::endl;
    std::cout << "Do you want to list all the days? (y/n)";
    std::string answer;
    std::cin >> answer;
    if ((answer.compare("y") == 0) || (answer.compare("Y") == 0))
    {
        for (int i = 0; i <= my_life.size(); i++)
        {
            std::cout << my_life[i].get_date_str() << '\n';
        }
    }

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
        void remove(const std::string &date_freq);
        bool is_last_day_in_month() const;
        bool is_leap_year() const;
};

// external functions
std::vector<myDate> create_date_serie(const std::string &date_str_begin, const std::string &date_str_end, const std::string &date_freq, const std::string &date_format);
double eval_freq(const std::string &freq);
int get_nearest_idx(const myDate &date, const std::vector<myDate> &dates, const std::string &type);
