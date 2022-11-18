#include <cmath>
#include <vector>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <stdexcept>
#include "lib_math.h"

/*
 * NORMAL DISTRIBUTION
 */

// see: https://stackoverflow.com/questions/2328258/cumulative-normal-distribution-function-in-c-c
std::vector<double> norm_cdf(const std::vector<double> &x)
{
    double M_SQRT_1_2 = std::sqrt(0.5);
    std::vector<double> cdf;
    for (int idx = 0; idx < x.size(); idx++)
    {
        cdf.push_back(0.5 * std::erfc(-x[idx] * M_SQRT_1_2));
    }
    return cdf;
}

std::vector<double> norm_pdf(const std::vector<double> &x)
{
    double PI = 3.14159265358979;
    std::vector<double> pdf;
    for (int idx = 0; idx < x.size(); idx++)
    {
        pdf.push_back(1 / std::sqrt(2 * PI) * std::exp(-0.5 * std::pow(x[idx], 2)));
    }
    return pdf;
}

// auxiliary function for norm_inv()
double rational_approximation(double t)
{
    // Abramowitz and Stegun formula 26.2.23.
    // the absolute value of the error should be less than 4.5 e-4.
    double c[] = {2.515517, 0.802853, 0.010328};
    double d[] = {1.432788, 0.189269, 0.001308};
    return t - ((c[2] * t + c[1]) * t + c[0]) / (((d[2] * t + d[1]) * t + d[0]) * t + 1.0);
}

// auxiliary function for norm_inv()
double norm_inv_aux(double p)
{
    if (p <= 0.0 || p >= 1.0)
    {
        std::stringstream os;
        os << "Invalid input argument (" << p << "); must be larger than 0 but less than 1.";
        throw std::invalid_argument(os.str());
    }

    if (p < 0.5)
    {
        // F^-1(p) = - G^-1(p)
        return -rational_approximation( sqrt(-2.0 * log(p)) );
    }
    else
    {
        // F^-1(p) = G^-1(1-p)
        return rational_approximation( sqrt(-2.0 * log(1 - p)) );
    }
}

// see: https://www.johndcook.com/blog/cpp_phi_inverse/
std::vector<double> norm_inv(const std::vector<double> &x)
{
    std::vector<double> p;
    for (int idx = 0; idx < x.size(); idx++)
    {
        p.push_back(norm_inv_aux(x[idx]));
    }
    return p;
}

/*
 * NEWTON-RAPHSON METHOD
 */
newton_raphson_res newton_raphson(double (*func)(double), double x, const double &step, const double &tolerance, const int &iter_max)
{
    // variables
    newton_raphson_res res;
    int iter_no = 0;
    double derivative;
    double x_down;
    double x_up;
    double func_down;
    double func_par;
    double func_up;

    // initiate Newton-Raphson structure
    res.x = x;
    res.step = step;
    res.tolerance = tolerance;
    res.iter_max = iter_max;

    do
    {
        // calculate derivation at initinal point x
        x_down = res.x - step;
        x_up = res.x + step;
        func_up = func(x_up);
        func_down = func(x_down);
        derivative = (func_up - func_down) / (2 * step);

        // apply Newton-Raphson method
        func_par = func(res.x);
        if (derivative == 0.0)
        {
            // return result after derivation being equal to zero
            res.success = false;
            res.iter_no = iter_no;
            res.msg = "zero derivation at point x = " + std::to_string(res.x);
            return res;
        }
        else if (std::abs(func_par) < tolerance)
        {
            // return result after reaching proximity of zero
            res.success = true;
            res.iter_no = iter_no;
            res.msg = "tolerance reached; f(x) = " + std::to_string(func_par);
            return res;
        }
        else
        {
            // iterate
            iter_no++;
            res.x += -func_par / derivative;
        }
    } while (iter_no < iter_max);

    // return result after exceeding maximum number of iterations
    res.success = false;
    res.iter_no = iter_no;
    res.msg = "maxium number of iterations reached";
    return res;

}