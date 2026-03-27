#pragma once

#include "models/ScoredTrade.h"
#include "models/Trade.h"
#include <map>
#include <vector>
#include <memory>

class Scorer {
public:
    Scorer(double minProbability, double minProfitAmount,
           double minAnnualReturn);
    virtual ~Scorer() = default;

    double minProfitAmount() const { return m_minProfitAmount; }

protected:
    double calculateImpliedSd(
        const Option& option, double underlyingPrice) const;

    double successProbability(
        const std::map<int, double>& plByPrice,
        double underlyingPrice, double sd, int dte) const;

    MaxProfitLoss calcMaxProfitLoss(
        const std::map<int, double>& plByPrice,
        const StandardDeviationPrices& sdPrices) const;

    double annualReturnScore(
        int dte, double probability,
        const std::map<int, double>& plByPrice,
        const StandardDeviationPrices& sdPrices,
        const Trade& trade) const;

    double scoreByNumProfitablePoints(
        const std::map<int, double>& plByPrice,
        const StandardDeviationPrices& sdPrices) const;

    std::map<int, double> calcProfitLossByPrice(
        const Trade& trade,
        const StandardDeviationPrices& sdPrices) const;

    double m_minProbability;
    double m_minProfitAmount;
    double m_minAnnualReturn;

private:
    std::vector<int> findProfitableRanges(
        const std::vector<int>& prices) const;

    static bool equivalent(double a, double b);
};
