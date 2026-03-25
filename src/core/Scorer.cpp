#include "Scorer.h"
#include "MathUtils.h"
#include <algorithm>
#include <cmath>
#include <numeric>

Scorer::Scorer(double minProbability, double minProfitAmount,
               double minAnnualReturn)
    : m_minProbability(minProbability)
    , m_minProfitAmount(minProfitAmount)
    , m_minAnnualReturn(minAnnualReturn)
{}

double Scorer::calculateImpliedSd(
    const Option& option, double underlyingPrice) const
{
    return underlyingPrice * option.impliedVolatility
        * std::sqrt(static_cast<double>(option.dte) / 365.0);
}

double Scorer::successProbability(
    const std::map<int, double>& plByPrice,
    double underlyingPrice, double sd) const
{
    std::vector<int> profitPrices;
    for (const auto& [price, pl] : plByPrice) {
        if (pl > m_minProfitAmount) {
            profitPrices.push_back(price);
        }
    }

    auto ranges = findProfitableRanges(profitPrices);
    double totalProb = 0.0;
    for (size_t i = 0; i + 1 < ranges.size(); i += 2) {
        totalProb += MathUtils::normalProbability(
            underlyingPrice, sd,
            static_cast<double>(ranges[i]),
            static_cast<double>(ranges[i + 1])) * 100.0;
    }
    return totalProb;
}

std::vector<int> Scorer::findProfitableRanges(
    const std::vector<int>& prices) const
{
    if (prices.empty()) return {};

    auto sorted = prices;
    std::sort(sorted.begin(), sorted.end());

    std::vector<int> ranges;
    int lastPrice = sorted.front();
    for (size_t i = 0; i < sorted.size(); ++i) {
        int price = sorted[i];
        if (ranges.empty()) {
            ranges.push_back(price);
            lastPrice = price;
            continue;
        }
        if (price != lastPrice + 1) {
            ranges.push_back(lastPrice);
            ranges.push_back(price);
        }
        lastPrice = price;
    }
    ranges.push_back(lastPrice);
    return ranges;
}

MaxProfitLoss Scorer::calcMaxProfitLoss(
    const std::map<int, double>& plByPrice,
    const StandardDeviationPrices& sdPrices) const
{
    double max1SdProfit = -1e18;
    double max2SdLoss = 1e18;
    for (const auto& [price, pl] : plByPrice) {
        int band = sdPrices.sdBand(static_cast<double>(price));
        if (band < 2 && pl > max1SdProfit) max1SdProfit = pl;
        if (band < 3 && pl < max2SdLoss) max2SdLoss = pl;
    }

    double ratio = (max2SdLoss < 0)
        ? max1SdProfit / std::abs(max2SdLoss)
        : max1SdProfit;

    int numLosses = 0, numMaxLoss = 0;
    int numProfits = 0, numMaxProfit = 0;
    for (const auto& [_, pl] : plByPrice) {
        if (pl < m_minProfitAmount) {
            ++numLosses;
            if (equivalent(pl, max2SdLoss)) ++numMaxLoss;
        } else {
            ++numProfits;
            if (equivalent(pl, max1SdProfit)) ++numMaxProfit;
        }
    }

    double score = static_cast<double>(std::max(numMaxLoss, 1))
        / std::max(numLosses, 1);

    return { ratio, numMaxProfit, max1SdProfit,
             numMaxLoss, max2SdLoss, score };
}

double Scorer::annualReturnScore(
    int dte, double probability,
    const std::map<int, double>& plByPrice,
    const StandardDeviationPrices& sdPrices,
    const Trade& trade) const
{
    auto weightedAvg = [&](const std::map<int, double>& pls) {
        if (pls.empty()) return 0.0;
        double sum = 0.0;
        for (const auto& [price, pl] : pls) {
            int band = sdPrices.sdBand(static_cast<double>(price));
            double mult = 0.0;
            if (band <= 1) {
                mult = 1.0;
            } else if (band == 2) {
                mult = (pl < m_minProfitAmount) ? 1.0 : 0.5;
            }
            sum += mult * pl;
        }
        return sum / static_cast<double>(pls.size());
    };

    std::map<int, double> profitPls, lossPls;
    for (const auto& [price, pl] : plByPrice) {
        if (pl > m_minProfitAmount)
            profitPls[price] = pl;
        else
            lossPls[price] = pl;
    }

    double profitAvg = weightedAvg(profitPls);
    double lossAvg = weightedAvg(lossPls);
    int tradesPerYear = std::max(365 / std::max(dte, 7), 1);
    double profit = tradesPerYear * (probability / 100.0) * profitAvg;
    double loss = tradesPerYear
        * ((100.0 - probability) / 100.0) * lossAvg;

    return ((profit + loss) * 100.0) / trade.requiredMargin() * 100.0;
}

double Scorer::scoreByNumProfitablePoints(
    const std::map<int, double>& plByPrice,
    const StandardDeviationPrices& sdPrices) const
{
    int weightedCount = 0;
    for (const auto& [price, pl] : plByPrice) {
        if (pl <= m_minProfitAmount) continue;
        int band = sdPrices.sdBand(static_cast<double>(price));
        switch (band) {
        case 0: case 1: weightedCount += 4; break;
        case 2: weightedCount += 2; break;
        case 3: weightedCount += 1; break;
        default: break;
        }
    }
    return (static_cast<double>(weightedCount)
        / plByPrice.size()) * 100.0;
}

std::map<int, double> Scorer::calcProfitLossByPrice(
    const Trade& trade,
    const StandardDeviationPrices& sdPrices) const
{
    std::map<int, double> result;
    int lower = static_cast<int>(sdPrices.lowerSd);
    int upper = static_cast<int>(sdPrices.upperSd);
    for (int p = lower; p <= upper; ++p) {
        result[p] = trade.profitLossAtPrice(static_cast<double>(p));
    }
    return result;
}

bool Scorer::equivalent(double a, double b) {
    return std::abs(a - b) < 0.01;
}
