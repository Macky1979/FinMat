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
    vector<float> * Y = new vector<float>();

    // x and y are supposed to be orderd in ascending order
    vector<float> x = {0, 1, 2, 3, 4, 5}; 
    vector<float> y = {0, 1, 3, 5, 6, 7};
    vector<float> X = {-1, 1, 2, 8, 2.5, 1./3};

    // create interpolation object and interpolate
    myLinInterp interp(x, y);
    Y = interp.eval(X);

    // print out results
    for (int i = 0; i < Y->size(); i++)
    {
        cout << "Y[" + to_string(i) + "] = " + to_string((*Y)[i]) << std::endl;
    }

    // delete pointer
    delete Y;

    // everything OK
    return 0;
}
*/

// define object that handles linear interpolations
class myLinInterp
{
    private:
        // variables
        std::vector<float> x;
        std::vector<float> y;
        std::vector<float> X;
        std::vector<float> Y;

    public:
        // object constructors
        myLinInterp(const std::vector<float> &_x, const std::vector<float> &_y);

        // object destructor
        ~myLinInterp(){};

        // object function declarations
        std::vector<float> * eval(const std::vector<float> &_X);
};