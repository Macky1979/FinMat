#include <string>
#include <iostream>
#include "lib_date.h"

using namespace std;

int main()
{
    /*
    string date_format = "dd/mm/yyyy";
    string date_str_1 = "30/12/1979";
    string date_str_2 = "29/02/2000";
    myDate *my_date_1;
    myDate my_date_2(date_str_2, date_format);
    my_date_1 = &my_date_2;
    cout << my_date_1->get_days_no() << endl;
    cout << (*my_date_1).get_days_no() << endl;

    myDate my_date_2(date_str_2, date_format);
    cout << my_date_1.get_days_no() << endl;
    cout << my_date_2.get_days_no() << endl;
    cout << my_date_2 - my_date_1 << endl;

    cout << "*****" << endl;

    date_str_1 = "06/11/2021";
    cout << my_date_1.get_date_str() << endl;

    cout << "*****" << endl;

    int date_int_1 = 19791230;
    myDate my_date_3(date_int_1);
    cout << my_date_3.get_date_str() << endl;
    cout << my_date_3.get_date_int() << endl;

    date_int_1 = 20211106;
    cout << my_date_3.get_date_str() << endl;
    cout << my_date_3.get_date_int() << endl;    

    delete &my_date_1;
    delete my_date_2;
    delete my_date_3;


    int * date;
    int year = 1980;
    int month = 2;
    int day = 29;
    int months_to_add = 121;

    date = add_months(year, month, day, months_to_add);
    cout << date[0] << endl;
    cout << date[1] << endl;
    cout << date[2] << endl;
 
    cout << "*****" << endl;

    year = 1979;
    month = 12;
    day = 31;
 
    months_to_add = 491;
    date = add_months(year, month, day, months_to_add);
    cout << date[0] << endl;
    cout << date[1] << endl;
    cout << date[2] << endl;

   */

    string date_format = "dd/mm/yyyy";
    string date_str_1 = "30/12/1979";
    myDate my_date_1(date_str_1, date_format);
    my_date_1.add("8523D");
    cout << my_date_1.get_date_str() << endl;

    return 0;
}