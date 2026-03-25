#pragma once

#include "Trade.h"
#include <map>
#include <memory>

struct StandardDeviationPrices {
    double underlyingPrice = 0.0;
    double standardDeviation = 0.0;
    double upperSd = 0.0;
    double lowerSd = 0.0;

    StandardDeviationPrices() = default;
    StandardDeviationPrices(double price, double sd)
        : underlyingPrice(price)
        , standardDeviation(sd)
        , upperSd(price + 2.0 * sd)
        , lowerSd(std::max(price - 2.0 * sd, 1.0))
    {}

    int sdBand(double price) const {
        double diff = std::abs(underlyingPrice - price);
        return static_cast<int>(diff / standardDeviation) + 1;
    }
};

struct MaxProfitLoss {
    double maxProfitToMaxLossRatio = 0.0;
    int numMaxProfit = 0;
    double maxProfit = 0.0;
    int numMaxLoss = 0;
    double maxLoss = 0.0;
    double score = 0.0;
};

struct Score {
    double pricePointScore = 0.0;
    double numProfitablePointsScore = 0.0;
    double probability = 0.0;
    double maxLossRatio = 0.0;
    double annualizedReturn = 0.0;
    double deltaScore = 0.0;
    int hundredTradesScore = 0;
};

struct RawScoredTrade {
    Score score;
    std::map<int, double> plByPrice;
    StandardDeviationPrices sdPrices;
    MaxProfitLoss maxProfitLoss;
    double tradeDelta = 0.0;
    std::shared_ptr<Trade> trade;
};

struct ScoredTrade {
    int score = 0;
    std::map<int, double> plByPrice;
    StandardDeviationPrices sdPrices;
    double successProbability = 0.0;
    MaxProfitLoss maxProfitLoss;
    double annualizedReturn = 0.0;
    double tradeDelta = 0.0;
    int hundredTrades = 0;
    std::shared_ptr<Trade> trade;
};
