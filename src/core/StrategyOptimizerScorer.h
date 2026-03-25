#pragma once

#include "Scorer.h"
#include "models/ScoredTrade.h"
#include <vector>
#include <memory>

class StrategyOptimizerScorer : public Scorer {
public:
    explicit StrategyOptimizerScorer(
        double minProbability = 25.0,
        double minProfitAmount = 0.5,
        double minAnnualReturn = 1.0);

    std::vector<ScoredTrade> score(
        const std::vector<std::shared_ptr<Trade>>& trades,
        double underlyingPrice);

private:
    RawScoredTrade* scoreSingle(
        const std::shared_ptr<Trade>& trade,
        double underlyingPrice);

    double calculateOverallImpliedSd(
        const Trade& trade, double underlyingPrice) const;

    double scoreByPricePoints(
        const std::map<int, double>& plByPrice,
        double underlyingPrice, double sd) const;

    struct Deltas {
        double tradeDelta;
        double deltaScore;
    };
    Deltas calcDeltas(const Trade& trade) const;

    int hundredTrades(
        const std::map<int, double>& plByPrice,
        double probability) const;

    std::vector<ScoredTrade> normalize(
        const std::vector<RawScoredTrade>& raw);
};
