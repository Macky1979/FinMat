#pragma once

#include <iostream>
#include <string>
#include <vector>

// user defined datatype to hold result of SQL query
struct dataFrame
{
	std::vector<std::string> col_nms;
	std::vector<std::string> dtypes;
	std::vector<std::vector<std::string>> values; 
};

// dataframe object
class myDataFrame
{
	private:
	
	public:
		// data structure describing dataframe
		dataFrame tbl;

		// object constructors
		myDataFrame(){};
		myDataFrame(dataFrame _tbl){tbl = _tbl;};

		// object destructor
		~myDataFrame(){};

		//object function declarations
		void clear();
		const long get_rows_no() const;
		const int get_cols_no() const;
};