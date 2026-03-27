#pragma once

namespace MathUtils {

double normalCdf(double x);
double normalProbability(double mean, double sd, double lower, double upper);
double lognormalProbability(double underlyingPrice, double sigmaRootT,
                           double T, double lower, double upper,
                           double riskFreeRate = 0.05);
double lognormalCdf(double price, double underlyingPrice,
                    double sigmaRootT, double T,
                    double riskFreeRate = 0.05);

} // namespace MathUtils
