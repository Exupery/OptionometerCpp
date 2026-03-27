#include "ScreenerService.h"
#include "core/TradeBuilder.h"
#include "core/StrategyOptimizerScorer.h"
#include "core/BullPutScorer.h"
#include "core/NakedPutScorer.h"
#include "core/Weigher.h"
#include "models/Trade.h"
#include "services/MarketDataImporter.h"
#include <algorithm>

ScreenerResult ScreenerService::runScreen(
    const std::vector<OptionChain>& chains,
    const ScreenerParams& params)
{
    Trade::setCommissionPerShare(params.commission / 100.0);

    if (params.mode == ScreenerMode::StrategyOptimizer) {
        return scoreTrades(chains, params);
    }
    if (params.mode == ScreenerMode::NakedPutSell) {
        return scoreNakedPuts(chains, params);
    }
    return scoreBullPuts(chains, params);
}

std::vector<std::vector<ScoredTrade>>
ScreenerService::scoreTrades(
    const std::vector<OptionChain>& chains,
    const ScreenerParams& params)
{
    Weigher weigher(
        params.pricePointWeight, params.profitPointWeight,
        params.probabilityWeight, params.profitLossWeight,
        params.annualReturnWeight, params.deltaWeight,
        params.hundredTradeWeight);
    StrategyOptimizerScorer scorer(
        params.minProbability, params.minProfitAmount,
        params.minAnnualReturn, weigher);

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

        MarketDataImporter::appendToLog(
            "  scoreTrades chain: calls=" + QString::number(chain.calls.size())
            + " puts=" + QString::number(chain.puts.size())
            + " trades=" + QString::number(trades.size()));

        auto scored = scorer.score(trades, chain.underlyingPrice);

        MarketDataImporter::appendToLog(
            "  scored=" + QString::number(scored.size()));

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
    Weigher weigher(
        params.pricePointWeight, params.profitPointWeight,
        params.probabilityWeight, params.profitLossWeight,
        params.annualReturnWeight, params.deltaWeight,
        params.hundredTradeWeight);
    BullPutScorer scorer(
        params.minProbability, params.minProfitAmount,
        params.minAnnualReturn, params.maxMargin, weigher);

    double underlyingPrice = chains.empty()
        ? 0.0 : chains.front().underlyingPrice;
    double minBelow = params.minBullPutStrikeBelow / 100.0;
    int defaultTarget = static_cast<int>(
        underlyingPrice - (underlyingPrice * minBelow));
    int targetStrike = (params.targetSellStrike > 0)
        ? params.targetSellStrike : defaultTarget;

    MarketDataImporter::appendToLog(
        "scoreBullPuts: chains=" + QString::number(chains.size())
        + " underlyingPrice=" + QString::number(underlyingPrice, 'f', 2)
        + " targetStrike=" + QString::number(targetStrike)
        + " minBelow=" + QString::number(params.minBullPutStrikeBelow));

    std::vector<std::vector<ScoredBullPut>> allResults;
    for (const auto& chain : chains) {
        TradeBuilder tb(chain);
        auto allBullPuts = tb.bullPutSpreads();

        int aboveUnderlying = 0;
        int belowMinPct = 0;
        int wrongTarget = 0;
        std::vector<std::shared_ptr<Trade>> filtered;
        for (const auto& trade : allBullPuts) {
            double shortStrike = trade->getSells().front().strike;
            if (shortStrike > underlyingPrice) {
                ++aboveUnderlying;
                continue;
            }

            if (params.mode == ScreenerMode::BullPutSpreadScreener) {
                double diff = underlyingPrice - shortStrike;
                double pct = (diff / underlyingPrice) * 100.0;
                if (pct >= params.minBullPutStrikeBelow)
                    filtered.push_back(trade);
                else
                    ++belowMinPct;
            } else {
                if (static_cast<int>(shortStrike) == targetStrike)
                    filtered.push_back(trade);
                else
                    ++wrongTarget;
            }
        }

        MarketDataImporter::appendToLog(
            "  Chain puts=" + QString::number(chain.puts.size())
            + " combos=" + QString::number(allBullPuts.size())
            + " aboveUnderlying=" + QString::number(aboveUnderlying)
            + " belowMinPct=" + QString::number(belowMinPct)
            + " wrongTarget=" + QString::number(wrongTarget)
            + " filtered=" + QString::number(filtered.size()));

        auto scored = scorer.score(filtered, underlyingPrice);

        MarketDataImporter::appendToLog(
            "  scored=" + QString::number(scored.size()));

        std::sort(scored.begin(), scored.end(),
            [](const ScoredBullPut& a, const ScoredBullPut& b) {
                return a.score > b.score;
            });
        allResults.push_back(std::move(scored));
    }
    return allResults;
}

std::vector<std::vector<ScoredBullPut>>
ScreenerService::scoreNakedPuts(
    const std::vector<OptionChain>& chains,
    const ScreenerParams& params)
{
    Weigher weigher(
        params.pricePointWeight, params.profitPointWeight,
        params.probabilityWeight, params.profitLossWeight,
        params.annualReturnWeight, params.deltaWeight,
        params.hundredTradeWeight);
    NakedPutScorer scorer(
        params.minProbability, params.minProfitAmount,
        params.minAnnualReturn, params.maxNakedPutMargin, weigher);

    double underlyingPrice = chains.empty()
        ? 0.0 : chains.front().underlyingPrice;
    double minBelow = params.minNakedPutStrikeBelow / 100.0;
    double maxStrike = underlyingPrice * (1.0 - minBelow);

    MarketDataImporter::appendToLog(
        "scoreNakedPuts: chains=" + QString::number(chains.size())
        + " underlyingPrice=" + QString::number(underlyingPrice, 'f', 2)
        + " maxStrike=" + QString::number(maxStrike, 'f', 2)
        + " minBelow=" + QString::number(params.minNakedPutStrikeBelow));

    std::vector<std::vector<ScoredBullPut>> allResults;
    for (const auto& chain : chains) {
        TradeBuilder tb(chain);
        auto allNakedPuts = tb.nakedPuts();

        int aboveThreshold = 0;
        std::vector<std::shared_ptr<Trade>> filtered;
        for (const auto& trade : allNakedPuts) {
            double strike = trade->getSells().front().strike;
            if (strike > maxStrike) {
                ++aboveThreshold;
                continue;
            }
            filtered.push_back(trade);
        }

        MarketDataImporter::appendToLog(
            "  Chain puts=" + QString::number(chain.puts.size())
            + " nakedPuts=" + QString::number(allNakedPuts.size())
            + " aboveThreshold=" + QString::number(aboveThreshold)
            + " filtered=" + QString::number(filtered.size()));

        auto scored = scorer.score(filtered, underlyingPrice);

        MarketDataImporter::appendToLog(
            "  scored=" + QString::number(scored.size()));

        std::sort(scored.begin(), scored.end(),
            [](const ScoredBullPut& a, const ScoredBullPut& b) {
                return a.score > b.score;
            });
        allResults.push_back(std::move(scored));
    }
    return allResults;
}
