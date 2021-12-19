#include <string>
#include <iostream>
#include <tuple>
#include <vector>
#include <memory>
#include "lib_date.h"

/*
 * GENERAL FUNCTIONS
 */

// determine number of days in month
static int days_in_month(const int &year, const int &month)
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
            throw std::invalid_argument((std::string)__func__ + ": " + std::to_string(month) + "  is not supported month!" );
    }
}

// add months
static int * add_months(const int &year, const int &month, const int &day, const int &months_to_add)
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
    if (day < _day)
        _day = day;

    // return result
    date[0] = _year;
    date[1] = _month;
    date[2] = _day;

    return date;
}

// remove months
static int * remove_months(const int &year, const int &month, const int &day, const int &months_to_remove)
{
    // declare array of integers to hold information on year, month and day
    static int date[3];

    // individual date components
    int _years_to_remove;
    int _year;
    int _month;
    int _day;

    // determine how many years and months should be removed straight away
    _years_to_remove = months_to_remove / 12;
    _month = months_to_remove % 12;

    // determine month
    if ((_month > 0) && (month - _month) < 1)
    {
        _years_to_remove++;
        _month = (12 + month - _month);
    }
    else
    {
        _month = month - _month;
    }

    // determine year
    _year = year - _years_to_remove;

    // check maximum day in month
    _day = days_in_month(_year, _month);
    if (day < _day)
        _day = day;

    // return result
    date[0] = _year;
    date[1] = _month;
    date[2] = _day;

    return date;
}

// decompose date frequency of (for example) "6M" into 6 and "M"
static std::tuple<int, std::string> decompose_freq(const std::string &freq)
{
   // variables to hold frequency type (e.g. D, M, Y) and number of frequency units
   int freq_no;
   std::string freq_type; 

   // determine frequency units
   freq_no = stoi(freq.substr(0, freq.size() - 1));

   // determine frequency type
   freq_type = freq.substr(freq.size() - 1, 1);

   // return number of frequency units and frequency type
   return {freq_no, freq_type};
 }

// create a std::vector of dates from start date to end date using time step of a given frequency
std::vector<myDate> * create_date_serie(const std::string &date_str_begin, const std::string &date_str_end, const std::string &date_freq, const std::string &date_format)
{
    // create std::vector to hold date serie
    std::vector<myDate> * date_serie = new std::vector<myDate>();

    //create date end and current date objects
    myDate date_end(date_str_end, date_format);
    myDate date_current(date_str_begin, date_format);

    // create date series
    while (date_current.get_date_int() < date_end.get_date_int())
    {
        (*date_serie).push_back(date_current);
        date_current.add(date_freq);
    }

    // return pointer to the std::vector with date serie
    return date_serie;
}

// convert date frequency std::string into approximate year fraction; could
// be used to compare individual date frequencies
float eval_freq(const std::string &freq)
{
    int freq_no;
    std::string freq_type;

    if (freq_type.compare("D") == 0)
    {
        return freq_no / 365.25;
    }
    else if (freq_type.compare("M") == 0)
    {
        return 30.438 * freq_no / 365.25;
    }
    else if (freq_type.compare("Y") == 0)
    {
        return freq_no;
    }
    else
    {
        throw std::runtime_error((std::string)__func__ + ": unsupported date frequency type " + freq_type + "!");
    }
}

/*
 * OBJECT FUNCTIONS
 */

// set year, month and day based on date std::string in yyyymmdd format
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

// update date integer, year, month and day based on date std::string in format yyyymmdd
void myDate::recalc()
{
    // convert yyyymmdd date std::string into integer
    date_int = stoi(date_str);

    // set year, month and day
    this->set_year_month_day();

    // set days_no
    this->set_days_no();
}

//shift date forward using specified date frequency (e.g. 2D, 3M, 10Y)
void myDate::add(const std::string &date_freq)
{
    // variables to hold frequency type (e.g. D, M, Y) and number of frequency units
    int date_freq_no;
    std::string date_freq_type;
    std::tie(date_freq_no, date_freq_type) = decompose_freq(date_freq); 

    if (date_freq_type.compare("D") == 0)
    {
        int _days_no;
        int _years_to_add;
        int _months_to_add;
        int _days_in_month;
        std::string _freq_date;

        // store number of days in the current date
        _days_no = this->days_no;

        // add years to the current days
        _years_to_add = date_freq_no / 366;
        if (_years_to_add != 0)
        {
            _days_no = this->get_days_no();
            _freq_date = std::to_string(_years_to_add) + "Y";
            this->add(_freq_date);
            date_freq_no -= this->days_no - _days_no;
        }

        // add months in bulk
        _months_to_add = date_freq_no / 31;
        if (_months_to_add != 0)
        {
            _days_no = this->get_days_no();
            _freq_date = std::to_string(_months_to_add) + "M";
            this->add(_freq_date);
            date_freq_no -= this->days_no - _days_no;
        }

        // add the last one or two months
        _days_in_month = days_in_month(this->year, this->month);
        while (date_freq_no + this->day > _days_in_month)
        {
            _days_no = this->get_days_no();
            _freq_date = "1M";
            this->add(_freq_date);
            date_freq_no -= this->days_no - _days_no;
            _days_in_month = days_in_month(this->year, this->month);
        }
        
        // add remaining days
        this->day += date_freq_no;
        this->date_str = std::to_string(this->year * 10000 + this->month * 100 + this->day);
        this->recalc();

    }
    else if (date_freq_type.compare("M") == 0)
    {
        int * date;
        date = add_months(this->year, this->month, this->day, date_freq_no);
        this->year = date[0];
        this->month = date[1];
        this->day = date[2];
        this->date_str = std::to_string(this->year * 10000 + this->month * 100 + this->day);
        this->recalc();
    }
    else if (date_freq_type.compare("Y") == 0)
    {
        this->year += date_freq_no;
        this->date_str = std::to_string(this->year * 10000 + this->month * 100 + this->day);
        this->recalc();
    }
    else
    {
        throw std::invalid_argument((std::string)__func__ + ": " + date_freq_type + " is  notsupported date date frequency type!");
    }
}

//shift date forward using specified date frequency (e.g. 2D, 3M, 10Y)
void myDate::remove(const std::string &date_freq)
{
    // variables to hold frequency type (e.g. D, M, Y) and number of frequency units
    int date_freq_no;
    std::string date_freq_type;

    tie(date_freq_no, date_freq_type) = decompose_freq(date_freq);

    if (date_freq_type.compare("D") == 0)
    {
        int _days_no;
        int _years_to_remove;
        int _months_to_remove;
        int _days_in_month;
        std::string _freq_date;

        // store number of days in the current date
        _days_no = this->days_no;

        // remove years from the current days
        _years_to_remove = date_freq_no / 365;
        if (_years_to_remove != 0)
        {
            _days_no = this->get_days_no();
            _freq_date = std::to_string(_years_to_remove) + "Y";
            this->remove(_freq_date);
            date_freq_no -= _days_no - this->days_no;
        }

        // remove months in bulk
        _months_to_remove = date_freq_no / 30;
        if (_months_to_remove != 0)
        {
            _days_no = this->get_days_no();
            _freq_date = std::to_string(_months_to_remove) + "M";
            this->remove(_freq_date);
            date_freq_no -= _days_no - this->days_no;
        }

        // remove the last one or two months
        while (this->day - date_freq_no < 0)
        {
            date_freq_no -= this->day;
            this->month--;
            _days_in_month = days_in_month(this->year, this->month);
            this->day = _days_in_month;
            if (this->month < 0)
            {
                this->year--;
                this->month += 12;
            }
            this->date_str = std::to_string(this->year * 10000 + this->month * 100 + this->day);
            this->recalc();
        }
        this->day -= date_freq_no;
        this->date_str = std::to_string(this->year * 10000 + this->month * 100 + this->day);
        this->recalc();            
    }
    else if (date_freq_type.compare("M") == 0)
    {
        int * date;
        date = remove_months(this->year, this->month, this->day, date_freq_no);
        this->year = date[0];
        this->month = date[1];
        this->day = date[2];
        this->date_str = std::to_string(this->year * 10000 + this->month * 100 + this->day);
        this->recalc();
    }
    else if (date_freq_type.compare("Y") == 0)
    {
        year -= date_freq_no;
        this->date_str = std::to_string(this->year * 10000 + this->month * 100 + this->day);
        this->recalc();
    }
    else
    {
        throw std::invalid_argument((std::string)__func__ + ": " + date_freq_type + " is  notsupported date date frequency type!");
    }
}

bool myDate::is_last_day_in_month() const
{
    // copy myDate object into an auxiliary variable
    myDate date_aux = myDate(this->get_date_int());

    // store the current month into an auxiliary variable
    int month_aux = date_aux.get_month();

    // add one date to the date
    date_aux.add("1D");

    // check if the month has changed
    if (date_aux.get_month() > month_aux){
        return true;
    }
    else
    {
        return false;
    }
}

bool myDate::is_leap_year() const
{
    // variables
    bool is_leap = false;
    int year = this->get_year();

    // the most common defintion of a leap year is divisibility by 4
    if (year % 4 == 0)
    {
        is_leap = true;
    }

    // however we skip leap year every 100 years
    if (year % 100 == 0)
    {
        is_leap = false;
    }

    // unless the year is not divisible by 400
    if (year % 400 == 0)
    {
        is_leap = false;
    }

    // return information if the year is leap year or not
    return is_leap;
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

myDate::myDate(const std::string &date_str, const std::string &date_format)
{
    // convert date std::string into yyyymmdd string format
    if (date_format.compare("dd/mm/yyyy") == 0)
    {
        this->date_str = date_str.substr(6,4) + date_str.substr(3,2) + date_str.substr(0, 2);
    }
    else if (date_format.compare("yyyy/mm/dd") == 0)
    {
        this->date_str = date_str.substr(0,4) + date_str.substr(5,2) + date_str.substr(8, 2);
    }
    else if (date_format.compare("ddmmyyyy") == 0)
    {
        this->date_str = date_str.substr(4, 4) + date_str.substr(2, 2) + date_str.substr(0, 2);
    }
    else if (date_format.compare("yyyymmdd") == 0)
    {
        this->date_str = date_str;
    }
    else
    {
        throw std::invalid_argument((std::string)__func__ + ": " + date_format + " is not a supported date string format!");
    }

    // determine year, month, day and number of days since 01/01/1601
    this->recalc();

}

myDate::myDate(const int &date_int)
{
    // store date integer; we assume yyyymmdd format
    this->date_int = date_int;

    // store date as std::string in yyyymmdd
    this->date_str = std::to_string(date_int);

    // determine year, month, day and number of days since 01/01/1601
    this->recalc();
}

/*
 * COPY CONSTRUCTOR
 */

myDate::myDate (const myDate &date)
{
    this->date_str = date.get_date_str();
    this->recalc();
};

/*
 * OBJECT OPERATORS
 */

long myDate::operator- (const myDate &date2)
{
    return days_no - date2.get_days_no();
}

myDate& myDate::operator= (const myDate &date)
{
    this->date_str = date.get_date_str();
    this->recalc();
    return *this;
};

