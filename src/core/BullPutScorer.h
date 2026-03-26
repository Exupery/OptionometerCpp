#pragma once

#include "Scorer.h"
#include "Weigher.h"
#include "models/ScoredBullPut.h"
#include <vector>
#include <memory>

class BullPutScorer : public Scorer {
public:
    BullPutScorer(
        double minProbability = 25.0,
        double minProfitAmount = 0.5,
        double minAnnualReturn = 1.0,
        int maxMargin = 10000,
        const Weigher& weigher = Weigher());

    std::vector<ScoredBullPut> score(
        const std::vector<std::shared_ptr<Trade>>& trades,
        double underlyingPrice);

private:
    RawScoredBullPut* scoreSingle(
        const std::shared_ptr<Trade>& trade,
        double underlyingPrice);

    bool isValidBullPutSpread(const Trade& trade) const;

    MaxProfitLoss calcBullPutMaxProfitLoss(
        const Trade& trade, double probability,
        const std::map<int, double>& plByPrice) const;

    double bullPutAnnualReturn(
        const Trade& trade, const MaxProfitLoss& tradePl,
        double probability) const;

    std::vector<ScoredBullPut> normalize(
        const std::vector<RawScoredBullPut>& raw);

    int m_maxMargin;
    Weigher m_weigher;
};
