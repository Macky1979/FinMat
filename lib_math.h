/*
#include <string>
#include <vector>
#include <iostream>
#include <math.h>
#include "lib_math.h"

double sqrt_of_2(double x)
{
    return std::pow(x, 2) - 2;
}

int main()
{

    // NORMAL DISTRIBUTION
    std::vector<double> x = {-4.0, -3.0, -2.0, -1.0, 0.0, 1.0, 2.0, 3.0, 4.0};
    std::vector<double> pdf = norm_pdf(x);
    std::vector<double> cdf = norm_cdf(x);

    std::cout << "NORMAL DISTRIBUTION" << std::endl;
    for (int idx = 0; idx < x.size(); idx++)
    {
        std::cout << "x: " << std::to_string(x[idx]) << " -> pdf: " << std::to_string(pdf[idx]) << ", cdf: " << std::to_string(cdf[idx]) << std::endl;
    }

    std::vector<double> q = {0.001, 0.01, 0.10, 0.25, 0.50, 0.75, 0.90, 0.99, 0.999};
    std::vector<double> z = norm_inv(q);

    for (int idx = 0; idx < q.size(); idx++)
    {
        std::cout << "q: " << std::to_string(q[idx]) << " -> inv: " << std::to_string(z[idx])<< std::endl;
    }
    std::cout << '\n' << std::endl;

    // NEWTON-RAPHSON METHOD
    double X = 1.5;  // initial estimate of square root of 2
    double step = 1e-5;
    double tolerance = 1e-7;
    int iter_max = 1000;

    newton_raphson_res res = newton_raphson(sqrt_of_2, X, step, tolerance, iter_max);

    if (res.success)
        std::cout << "Newton-Raphson method: success" << std::endl;
    else
        std::cout << "Newton-Raphson method: failure" << std::endl;

    std::cout << "square root of 2 is " << std::to_string(res.x) << std::endl;
    std::cout << "step: " << std::to_string(res.step) << "; tolerance: " << std::to_string(res.tolerance) << "; iter_no: " << std::to_string(res.iter_no) << "; iter_max: " << std::to_string(res.iter_max) << std::endl;
    std::cout << "msg: " << res.msg << std::endl;
    std::cout << '\n' << std::endl;

    return 0;

}
*/

#pragma once

#include <string>
#include <vector>

// standardized normal distribution
std::vector<double> norm_cdf(const std::vector<double> &x);
std::vector<double> norm_pdf(const std::vector<double> &x);
std::vector<double> norm_inv(const std::vector<double> &x);

// Newton-Raphson method
struct newton_raphson_res
{
    double x;
    bool success;
    double step;
    double tolerance;
    int iter_no;
    int iter_max;
    std::string msg;
};
newton_raphson_res newton_raphson(double (*func)(double), double x, const double &step, const double &tolerance, const int &iter_max);
