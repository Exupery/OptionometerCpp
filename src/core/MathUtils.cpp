#include "MathUtils.h"
#define _USE_MATH_DEFINES
#include <algorithm>
#include <cmath>
#include <limits>

static constexpr double SQRT1_2 = 0.70710678118654752440;

namespace MathUtils {

double normalCdf(double x) {
    return 0.5 * std::erfc(-x * SQRT1_2);
}

double normalProbability(
    double mean, double sd, double lower, double upper)
{
    double lowerZ = (lower - mean) / sd;
    double upperZ = (upper - mean) / sd;
    return normalCdf(upperZ) - normalCdf(lowerZ);
}

double lognormalCdf(double price, double underlyingPrice,
                    double sigmaRootT, double T,
                    double riskFreeRate)
{
    if (price <= 0.0) return 0.0;
    if (sigmaRootT <= 0.0) return (price >= underlyingPrice) ? 1.0 : 0.0;
    double drift = (riskFreeRate - 0.5 * sigmaRootT * sigmaRootT / T) * T;
    double z = (std::log(price / underlyingPrice) - drift) / sigmaRootT;
    return normalCdf(z);
}

double lognormalProbability(double underlyingPrice, double sigmaRootT,
                            double T, double lower, double upper,
                            double riskFreeRate)
{
    return lognormalCdf(upper, underlyingPrice, sigmaRootT, T, riskFreeRate)
        - lognormalCdf(std::max(lower, 0.001), underlyingPrice, sigmaRootT, T, riskFreeRate);
}

double normalInverseCdf(double p) {
    if (p <= 0.0) return -std::numeric_limits<double>::infinity();
    if (p >= 1.0) return std::numeric_limits<double>::infinity();

    // Acklam's rational approximation for the inverse normal CDF
    static const double a[] = {
        -3.969683028665376e+01,  2.209460984245205e+02,
        -2.759285104469687e+02,  1.383577518672690e+02,
        -3.066479806614716e+01,  2.506628277459239e+00
    };
    static const double b[] = {
        -5.447609879822406e+01,  1.615858368580409e+02,
        -1.556989798598866e+02,  6.680131188771972e+01,
        -1.328068155288572e+01
    };
    static const double c[] = {
        -7.784894002430293e-03, -3.223964580411365e-01,
        -2.400758277161838e+00, -2.549732539343734e+00,
         4.374664141464968e+00,  2.938163982698783e+00
    };
    static const double d[] = {
         7.784695709041462e-03,  3.224671290700398e-01,
         2.445134137142996e+00,  3.754408661907416e+00
    };

    static const double pLow = 0.02425;
    static const double pHigh = 1.0 - pLow;

    double q, r;
    if (p < pLow) {
        q = std::sqrt(-2.0 * std::log(p));
        return (((((c[0]*q+c[1])*q+c[2])*q+c[3])*q+c[4])*q+c[5]) /
               ((((d[0]*q+d[1])*q+d[2])*q+d[3])*q+1.0);
    } else if (p <= pHigh) {
        q = p - 0.5;
        r = q * q;
        return (((((a[0]*r+a[1])*r+a[2])*r+a[3])*r+a[4])*r+a[5])*q /
               (((((b[0]*r+b[1])*r+b[2])*r+b[3])*r+b[4])*r+1.0);
    } else {
        q = std::sqrt(-2.0 * std::log(1.0 - p));
        return -(((((c[0]*q+c[1])*q+c[2])*q+c[3])*q+c[4])*q+c[5]) /
                ((((d[0]*q+d[1])*q+d[2])*q+d[3])*q+1.0);
    }
}

double lognormalInverseCdf(double p, double underlyingPrice,
                           double sigmaRootT, double T,
                           double riskFreeRate) {
    if (p <= 0.0) return 0.0;
    if (p >= 1.0) return std::numeric_limits<double>::infinity();
    if (sigmaRootT <= 0.0)
        return underlyingPrice * std::exp(riskFreeRate * T);
    double z = normalInverseCdf(p);
    double drift = (riskFreeRate - 0.5 * sigmaRootT * sigmaRootT / T) * T;
    return underlyingPrice * std::exp(z * sigmaRootT + drift);
}

} // namespace MathUtils
