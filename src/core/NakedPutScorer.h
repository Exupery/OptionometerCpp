#pragma once

#include "Scorer.h"
#include "Weigher.h"
#include "models/ScoredBullPut.h"
#include <vector>
#include <memory>

class NakedPutScorer : public Scorer {
public:
    NakedPutScorer(
        double minProbability = 25.0,
        double minProfitAmount = 0.5,
        double minAnnualReturn = 1.0,
        int maxMargin = 25000,
        const Weigher& weigher = Weigher(),
        int maxLossProbability = 5);

    std::vector<ScoredBullPut> score(
        const std::vector<std::shared_ptr<Trade>>& trades,
        double underlyingPrice);

    static double calcMarginPerContract(
        const Option& sellPut, double underlyingPrice);

    static int weekdaysInDte(int dte);

private:
    RawScoredBullPut* scoreSingle(
        const std::shared_ptr<Trade>& trade,
        double underlyingPrice);

    bool isValidNakedPut(const Trade& trade) const;

    MaxProfitLoss calcNakedPutMaxProfitLoss(
        const Trade& trade, double probability,
        const std::map<int, double>& plByPrice,
        double underlyingPrice,
        double sigmaRootT, double T) const;

    double nakedPutAnnualReturn(
        const Trade& trade, const MaxProfitLoss& tradePl,
        double probability, int numContracts,
        double marginPerContract,
        double underlyingPrice,
        double sigmaRootT, double T) const;

    std::vector<ScoredBullPut> normalize(
        const std::vector<RawScoredBullPut>& raw);

    int m_maxMargin;
    Weigher m_weigher;
    double m_maxLossProb;
};
