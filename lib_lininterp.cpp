#include <iostream>
#include <vector>
#include <tuple>
#include <math.h>
#include <algorithm>
#include "lib_lininterp.h"

/*
 * AUXILIARY FUNCTIONS
 */

// get the nearest surrounding grid points that will be used for 2D interpolation
std::vector<double> get_surrounding_grid_points(const std::vector<double> &x, const double &X)
{
    // vector with the surrounding grid points
    std::vector<double> surrounding_grid_points;

    // X is out of the range defined through vector x
    if (X <= x[0])
    {
        surrounding_grid_points.push_back(x[0]);
        surrounding_grid_points.push_back(x[0]);
        return surrounding_grid_points;
    }
    else if (X >= x[x.size() - 1])
    {
        surrounding_grid_points.push_back(x[x.size() - 1]);
        surrounding_grid_points.push_back(x[x.size() - 1]);
        return surrounding_grid_points;
    }

    // X is within the range defined through vector x
    double x_lower = x[0];
    double x_upper = x[x.size() - 1];
    for (int idx = 0; idx < x.size(); idx++)
    {
        // update lower bound
        if ((x[idx] <= X) & (x[idx] > x_lower))
        {
            x_lower = x[idx];
        }

        // update upper bound
        if ((x[idx] >= X) & (x[idx] < x_upper))
        {
            x_upper = x[idx];
        }
    }
    surrounding_grid_points.push_back(x_lower);
    surrounding_grid_points.push_back(x_upper);
    return surrounding_grid_points;
}

/*
 * OBJECT CONSTRUCTORS
 */

// 1D linear interpolation
myLinInterp::myLinInterp(const std::vector<double> &x, const std::vector<double> &y)
{
    // x and y are supposed to be orderd in ascending order; the assumption is not checked
    this->x = x;
    this->y = y;

    // check that arrays size are the same, e.g. there is one y for each x
    if (this->x.size() != this->y.size())
    {
        throw std::invalid_argument((std::string)__func__ + ": Vector x and y must of the same size!");
    }
}

// 2D linear interpolation
myLinInterp2D::myLinInterp2D(const std::vector<double> &x, std::vector<double> &y, const std::vector<double> &z)
{
    // vector x, y and z are supposed to be orderd in ascending order; the assumption is not checked
    this->x = x;
    this->y = y;
    this->z = z;

    // check that vectors x and y are of the same size, i.e. there is one x for each y
    if (x.size() != y.size())
    {
        throw std::invalid_argument((std::string)__func__ + ": Vector x and y must of the same size!");
    }

    // check that vectors x and z are of the same size, i.e. there is one x for each z
    if (x.size() != z.size())
    {
        throw std::invalid_argument((std::string)__func__ + ": Vector x and z must of the same size!");
    }
}

/*
 * OBJECT FUNCTIONS
 */

// 1D linear interpolation
std::vector<double> * myLinInterp::eval(const std::vector<double> &X)
{
    // create vector to hold interpolated values
    std::vector<double> * Y = new std::vector<double>[X.size()];

    // X vector for which Y vector is to be interpolated
    this->X = X;

    // go X by X
    for (int i = 0; i < X.size(); i++)
    {
        // check lower boundary
        if (this->X[i] <= this->x[0])
        {
            Y->push_back(this->y[0]);
            continue;
        }
        // check upper boundary
        else if (this->X[i] >= this->x[this->x.size() - 1])
        {
            Y->push_back(this->y[this->y.size() - 1]);
            continue;
        }
        // interior points
        else
        {
            // go x by x
            for (int j = 0; j < this->x.size() - 1; j++)
            {
                // check equality
                if (this->X[i] == this->x[j])
                {
                    Y->push_back(this->y[j]);
                    break;
                }
                // interpolate from the two nearest points
                else if ((this->X[i] > this->x[j]) && (this->X[i] < this->x[j + 1]))
                {
                    Y->push_back(this->y[j] + (this->y[j + 1] - this->y[j]) / (this->x[j + 1] - this->x[j]) * (this->X[i] - this->x[j]));
                    break;
                }

            }
        }
    }

    // return interpolated values
    return Y;

}

// 2D linear interpolation
std::vector<double> * myLinInterp2D::eval(const std::vector<double> &X, const std::vector<double> &Y)
{
    // check that vectors X and Y are of the same size, i.e. there is one X for each Y
    if (X.size() != Y.size())
    {
        throw std::invalid_argument((std::string)__func__ + ": Vector X and Y must of the same size!");
    }    

    // create vector to hold interpolated values
    std::vector<double> * Z = new std::vector<double>[X.size()];

    // XY vector couple for which Z vector is to be interpolated
    this->X = X;
    this->Y = Y;

    // go through XY grid points
    for (int X_idx = 0; X_idx < this->X.size(); X_idx++)
    {
        // get surrounding grid points
        std::vector<double> x_surrounding = get_surrounding_grid_points(this->x, this->X[X_idx]);
        std::vector<double> y_surrounding = get_surrounding_grid_points(this->y, this->Y[X_idx]);
        std::vector<std::vector<double>> grid_points;
 
        // get the grid points and their z component
        for (int x_idx = 0; x_idx < this->x.size(); x_idx++)
        {
            // the first grid point
            if ((x_surrounding[0] == x[x_idx]) & (y_surrounding[0] == y[x_idx]))
            {
                grid_points.push_back({x[x_idx], y[x_idx], z[x_idx]}); 
            }

            // the second grid point
            if ((x_surrounding[0] == x[x_idx]) & (y_surrounding[1] == y[x_idx]))
            {
                grid_points.push_back({x[x_idx], y[x_idx], z[x_idx]}); 
            }

            // the third grid point
            if ((x_surrounding[1] == x[x_idx]) & (y_surrounding[0] == y[x_idx]))
            {
                grid_points.push_back({x[x_idx], y[x_idx], z[x_idx]}); 
            }

            // the fourth grid point
            if ((x_surrounding[1] == x[x_idx]) & (y_surrounding[1] == y[x_idx]))
            {
                grid_points.push_back({x[x_idx], y[x_idx], z[x_idx]}); 
            }
        }

        // interpolate z
        double z_aux1;
        double z_aux2;
        double z_aux;
        
        // exact grid point match => no interpolation needed
        if ((grid_points[0][0] == grid_points[1][0]) & (grid_points[0][0] == grid_points[2][0]) & (grid_points[0][0] == grid_points[3][0]) &
            (grid_points[0][1] == grid_points[1][1]) & (grid_points[0][1] == grid_points[2][1]) & (grid_points[0][1] == grid_points[3][1]))
        {
            z_aux = grid_points[0][2];
        }
        // exact match for x-axis => z value interpolated from two grid points with different y value
        else if ((grid_points[0][0] == grid_points[1][0]) & (grid_points[0][0] == grid_points[2][0]) & (grid_points[0][0] == grid_points[3][0]) & (grid_points[2][1] != grid_points[0][1]))
        {
            z_aux = grid_points[0][2] + (grid_points[2][2] - grid_points[0][2]) / (grid_points[2][1] - grid_points[0][1]) * (Y[X_idx] - grid_points[0][1]);
        }
        // exact match for y-axis => z value interpolated from two grid points with different x value
        else if ((grid_points[0][1] == grid_points[1][1]) & (grid_points[0][1] == grid_points[2][1]) & (grid_points[0][1] == grid_points[3][1]) & (grid_points[2][0] != grid_points[0][0]))
        {
            z_aux = grid_points[0][2] + (grid_points[2][2] - grid_points[0][2]) / (grid_points[2][0] - grid_points[0][0]) * (X[X_idx] - grid_points[0][0]);
        }
        // z value interpolated from four different grid points
        else
        {
            z_aux1 = grid_points[0][2] + (grid_points[1][2] - grid_points[0][2]) / (grid_points[1][1] - grid_points[0][1]) * (Y[X_idx] - grid_points[0][1]);
            z_aux2 = grid_points[2][2] + (grid_points[3][2] - grid_points[2][2]) / (grid_points[3][1] - grid_points[2][1]) * (Y[X_idx] - grid_points[2][1]);
            z_aux = z_aux1 + (z_aux2 - z_aux1) / (grid_points[2][0] - grid_points[0][0]) * (X[X_idx] - grid_points[0][0]);
        }

        // add the interpolate z into a vector
        Z->push_back(z_aux);
    }
 
    // return interpolated values
    return Z;

}
