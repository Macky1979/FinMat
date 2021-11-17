#include <iostream>
#include "lib_dataframe.h"

using namespace std;

/*
 * OBJECT FUNCTIONS
 */

// clear content of the dataframe
void myDataFrame::clear()
{
    this->tbl.col_nms.clear();
    this->tbl.dtypes.clear();
    this->tbl.values.clear();
}

// get number of rows in the dataframe
const long myDataFrame::get_rows_no() const
{
    return long(this->tbl.values.size());
}

// get number of columns in the dataframe
const int myDataFrame::get_cols_no() const
{
    return int(this->tbl.col_nms.size());
}
