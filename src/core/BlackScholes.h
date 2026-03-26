#pragma once

#include "models/Trade.h"

namespace BlackScholes {

double callPrice(double S, double K, double T,
                 double sigma, double r = 0.05);
double putPrice(double S, double K, double T,
                double sigma, double r = 0.05);

double currentPlAtPrice(const Trade& trade,
                        double targetPrice, int dte);

} // namespace BlackScholes
