#include "BlackScholes.h"
#include "MathUtils.h"
#include <cmath>

namespace BlackScholes {

double callPrice(double S, double K, double T,
                 double sigma, double r)
{
    if (T <= 0.0) return std::max(S - K, 0.0);
    if (S <= 0.0) return 0.0;
    double sqrtT = std::sqrt(T);
    double d1 = (std::log(S / K) + (r + 0.5 * sigma * sigma) * T)
        / (sigma * sqrtT);
    double d2 = d1 - sigma * sqrtT;
    return S * MathUtils::normalCdf(d1)
        - K * std::exp(-r * T) * MathUtils::normalCdf(d2);
}

double putPrice(double S, double K, double T,
                double sigma, double r)
{
    if (T <= 0.0) return std::max(K - S, 0.0);
    if (S <= 0.0) return K * std::exp(-r * T);
    double sqrtT = std::sqrt(T);
    double d1 = (std::log(S / K) + (r + 0.5 * sigma * sigma) * T)
        / (sigma * sqrtT);
    double d2 = d1 - sigma * sqrtT;
    return K * std::exp(-r * T) * MathUtils::normalCdf(-d2)
        - S * MathUtils::normalCdf(-d1);
}

double currentPlAtPrice(const Trade& trade,
                        double targetPrice, int dte)
{
    double T = static_cast<double>(dte) / 365.0;
    double total = 0.0;

    for (const auto& opt : trade.getBuys()) {
        double bsValue = (opt.side == Side::Call)
            ? callPrice(targetPrice, opt.strike, T,
                        opt.impliedVolatility)
            : putPrice(targetPrice, opt.strike, T,
                       opt.impliedVolatility);
        double cost = opt.ask + Trade::commissionPerShare();
        total += bsValue - cost;
    }

    for (const auto& opt : trade.getSells()) {
        double bsValue = (opt.side == Side::Call)
            ? callPrice(targetPrice, opt.strike, T,
                        opt.impliedVolatility)
            : putPrice(targetPrice, opt.strike, T,
                       opt.impliedVolatility);
        double credit = opt.bid - Trade::commissionPerShare();
        total += credit - bsValue;
    }

    return total;
}

} // namespace BlackScholes
