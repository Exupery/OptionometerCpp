#include "MathUtils.h"
#define _USE_MATH_DEFINES
#include <algorithm>
#include <cmath>

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

} // namespace MathUtils
