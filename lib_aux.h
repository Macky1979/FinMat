#pragma once

#include <string>
#include <vector>

/*
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <vector>
#include "lib_aux.h"

int main()
{
    // cast to lower and upper case
    std::string x = "hello!";
    std::string y = "BYE!";

    std::cout << to_upper(x) << std::endl;
    std::cout << to_lower(y) << std::endl;

    // tic & toc
    tic();
    std::this_thread::sleep_for(std::chrono::seconds(3));
    std::cout << "We waited for approximately " + std::to_string(toc()) + " seconds!" << std::endl; 

    // time stamp
    std::cout << "time stamp: " + get_timestamp() << std::endl;

    // date stamp
    std::cout << "date stamp: " + get_datestamp() << std::endl;

    // vector split
    int vector_length = 1005;
    int splits_no = 4;
    std::vector<coordinates<int>> indicies = split_vector(vector_length, splits_no);

    // everything OK
    return 0;
}
*/

// co-ordinates structure
template <typename T>
struct coordinates
{
    T x;
    T y;
};

// convert string to an upper case
std::string to_upper(const std::string &input_str);

// convert string to an upper case
std::string to_lower(const std::string &input_str);

// implementation of tic()
void tic();

// implementation of toc()
double toc();

// implementation of timestamp
std::string get_timestamp();

// implementation of datestamp
std::string get_datestamp();

// split vector into several vectors of approximately same size => return indices which defines the new vectors
std::vector<coordinates<int>> split_vector(const int &vector_length, const int &splits_no);
