#pragma once

#include "ScoredTrade.h"
#include <memory>

struct RawScoredBullPut {
    Score score;
    std::map<int, double> plByPrice;
    StandardDeviationPrices sdPrices;
    MaxProfitLoss maxProfitLoss;
    int numContracts = 0;
    std::shared_ptr<Trade> trade;
};

struct ScoredBullPut {
    int score = 0;
    double successProbability = 0.0;
    MaxProfitLoss maxProfitLoss;
    double annualizedReturn = 0.0;
    int numContracts = 0;
    std::shared_ptr<Trade> trade;
};
