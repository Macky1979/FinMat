#pragma once

#include<string>
#include <algorithm>

/*
#include<iostream>
#include<string>

using namespace std;

int main()
{
    string x = "hello!";
    string y = "BYE!";

    cout << to_upper(x) << endl;
    cout << to_lower(y) << endl;

    return 0;

}

*/
// convert string to an upper case
std::string to_upper(const std::string &input_str);

// convert string to an upper case
std::string to_lower(const std::string &input_str);