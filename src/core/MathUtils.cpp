#include "MathUtils.h"
#define _USE_MATH_DEFINES
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

} // namespace MathUtils
