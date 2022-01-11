#include<string>
#include <algorithm>
#include <ctime>
#include <chrono>
#include <stack>
#include <vector>
#include <stdexcept>
#include "lib_aux.h"

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

// split vector into several vectors of approximately same size => return indices which defines the new vectors
std::vector<coordinates<int>> split_vector(const int &vector_length, const int &splits_no)
{
	// check that lenght of input vector is not lower than number of splits
	if (vector_length < splits_no)
	{
		throw std::invalid_argument((std::string)__func__ + ": Length of the input vector cannot be shorter than number of splits!");
	}

	// get number of elements per sub
	int elements_per_split = vector_length / splits_no;

	// create a vector holding position indicies of the newly created vectors
	std::vector<coordinates<int>> indicies;

	// determine position indicies of vectors created by the split of the original one
	int current_index = 0;
	for (int split_idx = 0; split_idx < splits_no; split_idx++)
	{
		// determine lower and upper position indicies of one particular split
		coordinates<int> index;
		index.x = current_index;
		current_index += elements_per_split;
		index.y = current_index - 1;

		// add the position indicies to the vector
		indicies.push_back(index);
	}

	// place the last remaining elements to the last vector
	indicies[splits_no - 1].y = vector_length - 1;

	// return vector with position indicies
	return indicies;
}