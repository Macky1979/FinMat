#include<string>
#include <algorithm>
#include <ctime>
#include <chrono>
#include <stack>

// global variable holding time elapsed between tic() and toc()
std::chrono::time_point<std::chrono::high_resolution_clock> tictoc_begin;

// convert string to an upper case
std::string to_upper(const std::string &input_str)
{
    // convert data type to upper case
    std::string _input_str = input_str;
    std::transform(_input_str.begin(), _input_str.end(), _input_str.begin(), ::toupper);
    
    // return upper-cased string
    return _input_str;
}

// convert string to an lower case
std::string to_lower(const std::string &input_str)
{
    // convert data type to upper case
    std::string _input_str = input_str;
    std::transform(_input_str.begin(), _input_str.end(), _input_str.begin(), ::tolower);
    
    // return upper-cased string
    return _input_str;
}

// implementation of tic()
void tic()
{
    tictoc_begin = std::chrono::high_resolution_clock::now();
}

// implementation of toc()
double toc()
{
    std::chrono::duration<double> elapsed_time (std::chrono::high_resolution_clock::now() - tictoc_begin);
    return elapsed_time.count();
}

// implementation of time stamp
std::string get_timestamp()
{
	// define variables
	time_t rawtime;
	struct tm * timeinfo;
	char char_aux [20];
	std::string timeStr = "";

	// get time
	time (&rawtime);
	timeinfo = localtime (&rawtime);

	// format time
    strftime (char_aux, 20, "%H:%M:%S", timeinfo);
	timeStr.assign(char_aux);

	// return time string
	return timeStr;
}

// implementation of date stamp
std::string get_datestamp()
{
	// define variables
	time_t rawtime;
	struct tm * timeinfo;
	char char_aux [20];
	std::string timeStr = "";

	// get time
	time (&rawtime);
	timeinfo = localtime (&rawtime);

	// format time
	strftime (char_aux, 20, "%d/%m/%Y %H:%M:%S", timeinfo);
	timeStr.assign(char_aux);

	// return time string
	return timeStr;
}