#include <string>
#include <iostream>
#include <vector>
#include <tuple>
#include "lib_lininterp.h"

int main()
{
    /*
     * 1D linear interpolation
     */

    // pointer to vector holding the interpolated figures
    std::vector<double> * Y1 = new std::vector<double>();

    // x and y are supposed to be orderd in ascending order
    std::vector<double> x1 = {0, 1, 2, 3, 4, 5}; 
    std::vector<double> y1 = {0, 1, 3, 5, 6, 7};
    std::vector<double> X1 = {-1, 1, 2, 8, 2.5, 1./3};

    // create interpolation object and interpolate
    myLinInterp interp(x1, y1);
    Y1 = interp.eval(X1);

    // print out results
    std::cout << "1D INTERPOLATION" << std::endl;
    for (int i = 0; i < Y1->size(); i++)
    {
        std::cout << "Y[" + std::to_string(i) + "] = " + std::to_string((*Y1)[i]) << std::endl;
    }
    std::cout << "----------------" << std::endl;

    // delete pointer
    delete[] Y1;

    /*
     * 2D linear interpolation
     */

    // pointer to vector holding the interpolated figures
    std::vector<double> * Z2 = new std::vector<double>();

    // vectors x, y and z are supposed to be orderd in ascending order
    std::vector<double> x2 = {2.0, 2.0, 2.0, 2.0, 2.0,
                              3.0, 3.0, 3.0, 3.0, 3.0,
                              4.0, 4.0, 4.0, 4.0, 4.0}; // x - maturity in years
    
    std::vector<double> y2 = {0.10, 0.25, 0.50, 0.75, 0.90,
                              0.10, 0.25, 0.50, 0.75, 0.90,
                              0.10, 0.25, 0.50, 0.75, 0.90};  // option delta

    std::vector<double> z2 = {0.0723, 0.0635, 0.0596, 0.0557, 0.0518,  // 2Y volatility
                              0.0812, 0.0717, 0.0650, 0.0583, 0.0516,  // 3Y volatility
                              0.0887, 0.0782, 0.0706, 0.0630, 0.0554}; // 5Y volatility

    // x and y co-ordinates for which volatility should be interpolated
    std::vector<double> X2 = {2.00, 3.00, 2.25, 2.25};
    std::vector<double> Y2 = {0.75, 0.60, 0.25, 0.60};

    // create interpolation object and interpolate
    myLinInterp2D interp2D(x2, y2, z2);
    Z2 = interp2D.eval(X2, Y2);

    // print out results
    std::cout << "2D INTERPOLATION" << std::endl;
    for (int i = 0; i < Z2->size(); i++)
    {
        std::cout << "Z[" + std::to_string(i) + "] = " + std::to_string((*Z2)[i]) << std::endl;
    }
    std::cout << "----------------" << std::endl;

    // delete pointer
    delete[] Z2;

    // everything OK
    return 0;
}