#include <iostream>
#include <fstream>
#include "lib_dataframe.h"

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

// auxiliary function that prepares a single row to be written into a text file
std::string process_row(const std::vector<std::string> cols, const std::string &sep, const bool &quotes)
{
    // std::string to hold the row
    std::string row = "";

    // go column by column
    for (int col_idx = 0; col_idx < cols.size(); col_idx++)
    {
        // check if each column should be "closed" within quotes
        if (quotes)
        {
            row += "\"" + cols[col_idx] + "\"";
        }
        else
        {
            row += cols[col_idx];
        }

        // add column separator
        row += sep;
    }

    // remove quotes and sepator at the end of the last column
    if (quotes)
    {
        row = row.substr(0, row.size() - 2);
        row += "\"";
    }
    else
    {
        row = row.substr(0, row.size() - 1);
    }



    // add end-of-line character
    row += '\n';

    // return row
    return row;
}

// write data frame into a .csv file
void myDataFrame::write(const std::string &file_nm, const std::string &sep, const bool &quotes) const
{
    // variable to hold row
    std::string row;

    // open file
    std::ofstream f;
    f.open(file_nm);

    // check that the file is indeed opened
    if (f.is_open())
    {
        // write column names
        row = process_row(tbl.col_nms, sep, quotes);
        f << row;

        // write data types
        row = process_row(tbl.dtypes, sep, quotes);
        f << row;   

        // go row by row
        for (long row_idx = 0; row_idx < tbl.values.size(); row_idx++)
        {
            row = process_row(tbl.values[row_idx], sep, quotes);
            f << row;
        }

        // close file
        f.close();
    }
    else
    {
        throw std::runtime_error((std::string)__func__ + ": Unable to open file " + file_nm + "!");
    }
}

// cut row read from .csv file into vector of cells
std::vector<std::string> * cut_row_into_cells(std::string row, std::string sep, const bool &quotes)
{
    // column separator position
    size_t pos;

    // variable to hold vector of cells
    std::vector<std::string> * cells = new std::vector<std::string>();

    // separator
    if (quotes)
    {
        sep = "\"" + sep + "\"";
    }

    // get rid of the leading and trailing quote
    if (quotes)
    {
        row = row.substr(1, row.size() - 1);
        row = row.substr(0, row.size() - 1);
    }

    // cut row into cells
    while (row.find(sep) != std::string::npos)
    {
        // get position of the column separator
        pos = row.find(sep);

        // cut a cell
        cells->push_back(row.substr(0, pos));

        // remove the cell from row
        row = row.substr(pos + sep.size(), row.size() - pos - sep.size());
    }
    cells->push_back(row);

    // return cells
    return cells;
}

// write data frame into a .csv file
void myDataFrame::read(const std::string &file_nm, const std::string &sep, const bool &quotes)
{
    // row read from the .csv file
    std::string row;

    // vector of cells
    std::vector<std::string> * cells;

    // open file
    std::ifstream f;
    f.open(file_nm, std::ios::out);

    // check that the file is indeed opened
    if (f.is_open())
    {
        // read columns names
        getline(f, row);
        cells = cut_row_into_cells(row, sep, quotes);
        tbl.col_nms = *cells;

        // read column types
        getline(f, row);
        cells = cut_row_into_cells(row, sep, quotes);
        tbl.dtypes = *cells;

        // read the values
        while (getline(f, row))
        {
            cells = cut_row_into_cells(row, sep, quotes);
            tbl.values.push_back(*cells);           
        }

        // close file
        f.close();
    }
    else
    {
        throw std::runtime_error((std::string)__func__ + ": Unable to open file " + file_nm + "!");
    }
}
