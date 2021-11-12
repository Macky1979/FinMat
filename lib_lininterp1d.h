#pragma once

#include <iostream>
#include <string>
#include <vector>

/*
#include <string>
#include <iostream>
#include "lib_lininterp.h"

using namespace std;

int main()
{
    // pointer to vector holding the interpolated figures
    vector<double> * Y = new vector<double>();

    // x and y are supposed to be orderd in ascending order
    vector<double> x = {0, 1, 2, 3, 4, 5}; 
    vector<double> y = {0, 1, 3, 5, 6, 7};
    vector<double> X = {-1, 1, 2, 8, 2.5, 1./3};

    // create interpolation object and interpolate
    myLinInterp interp(x, y);
    Y = interp.eval(X);

    // print out results
    for (int i = 0; i < Y->size(); i++)
    {
        cout << "Y[" + to_string(i) + "] = " + to_string((*Y)[i]) << '\n';
    }

    // delete pointer
    delete Y;

    // everything OK
    return 0;
}
*/

// define object that handles linear interpolations
class myLinInterp1d
{
    // private variables
    std::vector<double> x;
    std::vector<double> y;
    std::vector<double> X;
    std::vector<double> Y;

    public:
        // object constructors
        myLinInterp1d(const std::vector<double> &_x, const std::vector<double> &_y);

        // object destructor
        ~myLinInterp1d(){};

        // object function declarations
        std::vector<double> * eval(const std::vector<double> &_X);
};