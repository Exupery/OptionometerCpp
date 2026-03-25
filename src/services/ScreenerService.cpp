#include "ScreenerService.h"
#include "core/TradeBuilder.h"
#include "core/StrategyOptimizerScorer.h"
#include "core/BullPutScorer.h"
#include "core/Weigher.h"
#include "models/Trade.h"
#include <algorithm>

ScreenerResult ScreenerService::runScreen(
    const std::vector<OptionChain>& chains,
    const ScreenerParams& params)
{
    Trade::setCommissionPerShare(params.commission / 100.0);

    if (params.mode == ScreenerMode::StrategyOptimizer) {
        return scoreTrades(chains, params);
    }
    return scoreBullPuts(chains, params);
}

std::vector<std::vector<ScoredTrade>>
ScreenerService::scoreTrades(
    const std::vector<OptionChain>& chains,
    const ScreenerParams& params)
{
    StrategyOptimizerScorer scorer(
        params.minProbability, params.minProfitAmount,
        params.minAnnualReturn);

    std::vector<std::vector<ScoredTrade>> allResults;
    for (const auto& chain : chains) {
        TradeBuilder tb(chain);
        std::vector<std::shared_ptr<Trade>> trades;

        switch (params.tradeType) {
        case TradeType::TwoLeg:
            trades = tb.spreads();
            break;
        case TradeType::ThreeLeg:
            trades = tb.threeLegTrades();
            break;
        case TradeType::FourLeg:
            trades = tb.fourLegTrades();
            break;
        case TradeType::Condors:
            trades = tb.condors();
            break;
        case TradeType::BullPutSpreads:
            trades = tb.bullPutSpreads();
            break;
        }

        auto scored = scorer.score(trades, chain.underlyingPrice);
        std::sort(scored.begin(), scored.end(),
            [](const ScoredTrade& a, const ScoredTrade& b) {
                return a.score > b.score;
            });
        allResults.push_back(std::move(scored));
    }
    return allResults;
}

std::vector<std::vector<ScoredBullPut>>
ScreenerService::scoreBullPuts(
    const std::vector<OptionChain>& chains,
    const ScreenerParams& params)
{
    BullPutScorer scorer(
        params.minProbability, params.minProfitAmount,
        params.minAnnualReturn, params.maxMargin);

    double underlyingPrice = chains.empty()
        ? 0.0 : chains.front().underlyingPrice;
    double minBelow = params.minBullPutStrikeBelow / 100.0;
    int defaultTarget = static_cast<int>(
        underlyingPrice - (underlyingPrice * minBelow));
    int targetStrike = (params.targetSellStrike > 0)
        ? params.targetSellStrike : defaultTarget;

    std::vector<std::vector<ScoredBullPut>> allResults;
    for (const auto& chain : chains) {
        TradeBuilder tb(chain);
        auto allBullPuts = tb.bullPutSpreads();

        std::vector<std::shared_ptr<Trade>> filtered;
        for (const auto& trade : allBullPuts) {
            double shortStrike = trade->getSells().front().strike;
            if (shortStrike > underlyingPrice) continue;

            if (params.mode == ScreenerMode::BullPutSpreadScreener) {
                double diff = underlyingPrice - shortStrike;
                double pct = (diff / underlyingPrice) * 100.0;
                if (pct >= params.minBullPutStrikeBelow)
                    filtered.push_back(trade);
            } else {
                if (static_cast<int>(shortStrike) == targetStrike)
                    filtered.push_back(trade);
            }
        }

        auto scored = scorer.score(filtered, underlyingPrice);
        std::sort(scored.begin(), scored.end(),
            [](const ScoredBullPut& a, const ScoredBullPut& b) {
                return a.score > b.score;
            });
        allResults.push_back(std::move(scored));
    }
    return allResults;
}
