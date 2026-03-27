#pragma once

#include "models/Enums.h"
#include "models/OptionChain.h"
#include "models/ScoredTrade.h"
#include "models/ScoredBullPut.h"
#include <QMetaType>
#include <vector>
#include <variant>

struct ScreenerParams {
    QString ticker = "SPY";
    ScreenerMode mode = ScreenerMode::StrategyOptimizer;
    TradeType tradeType = TradeType::BullPutSpreads;
    int minProbability = 25;
    int minDays = 25;
    int maxDays = 45;
    bool fridaysOnly = false;
    int maxStrikes = 20;
    double commission = 0.5;
    double minAnnualReturn = 1.0;
    double minProfitAmount = 0.5;
    int pricePointWeight = 1;
    int profitPointWeight = 1;
    int probabilityWeight = 1;
    int profitLossWeight = 1;
    int annualReturnWeight = 1;
    int deltaWeight = 1;
    int hundredTradeWeight = 1;
    int maxMargin = 10000;
    int minBullPutStrikeBelow = 2;
    int targetSellStrike = 0;
    int maxNakedPutMargin = 25000;
    int minNakedPutStrikeBelow = 5;
    QString apiToken;
};

using ScreenerResult = std::variant<
    std::vector<std::vector<ScoredTrade>>,
    std::vector<std::vector<ScoredBullPut>>
>;

class ScreenerService {
public:
    static ScreenerResult runScreen(
        const std::vector<OptionChain>& chains,
        const ScreenerParams& params);

private:
    static std::vector<std::vector<ScoredTrade>> scoreTrades(
        const std::vector<OptionChain>& chains,
        const ScreenerParams& params);

    static std::vector<std::vector<ScoredBullPut>> scoreBullPuts(
        const std::vector<OptionChain>& chains,
        const ScreenerParams& params);

    static std::vector<std::vector<ScoredBullPut>> scoreNakedPuts(
        const std::vector<OptionChain>& chains,
        const ScreenerParams& params);
};

Q_DECLARE_METATYPE(ScreenerParams)
Q_DECLARE_METATYPE(ScreenerResult)
