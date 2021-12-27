#include <iostream>
#include <vector>
#include "lib_lininterp.h"

/*
 * OBJECT CONSTRUCTORS
 */

myLinInterp::myLinInterp(const std::vector<double> &_x, const std::vector<double> &_y)
{
    // x and y are supposed to be orderd in ascending order; the assumption is not checked
    x = _x;
    y = _y;

    // check that arrays size are the same, e.g. there is one y for each x
    if (x.size() != y.size())
    {
        throw std::invalid_argument((std::string)__func__ + ": Vector x and y must of the same size!");
    }
}

/*
 * OBJECT FUNCTIONS
 */

std::vector<double> * myLinInterp::eval(const std::vector<double> &_X)
{
    // create vector to hold interpolated values
    std::vector<double> * Y = new std::vector<double>();

    // X vector for which Y vector is to be interpolated
    X = _X;

    // auxiliary variable for interpolation


    // go X by X
    for (int i = 0; i < X.size(); i++)
    {
        // check lower boundary
        if (X[i] <= x[0])
        {
            Y->push_back(y[0]);
            continue;
        }
        // check upper boundary
        else if (X[i] >= x[x.size() - 1])
        {
            Y->push_back(y[y.size() - 1]);
            continue;
        }
        // interior points
        else
        {
            // go x by x
            for (int j = 0; j < x.size() - 1; j++)
            {
                // check equality
                if (X[i] == x[j])
                {
                    Y->push_back(y[j]);
                    break;
                }
                // interpolate from the two nearest points
                else if ((X[i] > x[j]) && (X[i] < x[j + 1]))
                {
                    Y->push_back(y[j] + (y[j + 1] - y[j]) / (x[j + 1] - x[j]) * (X[i] - x[j]));
                    break;
                }

            }
        }
    }

    // return interpolated values
    return Y;

}
