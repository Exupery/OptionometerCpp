#include "StrategyOptimizerScorer.h"
#include "Normalizer.h"
#include <algorithm>
#include <cmath>
#include <numeric>

static constexpr int MAX_HIGH_SCORES = 100;

StrategyOptimizerScorer::StrategyOptimizerScorer(
    double minProbability, double minProfitAmount,
    double minAnnualReturn, const Weigher& weigher)
    : Scorer(minProbability, minProfitAmount, minAnnualReturn)
    , m_weigher(weigher)
{}

std::vector<ScoredTrade> StrategyOptimizerScorer::score(
    const std::vector<std::shared_ptr<Trade>>& trades,
    double underlyingPrice)
{
    std::vector<RawScoredTrade> rawScored;
    for (const auto& trade : trades) {
        auto* raw = scoreSingle(trade, underlyingPrice);
        if (raw) {
            rawScored.push_back(std::move(*raw));
            delete raw;
        }
    }
    return normalize(rawScored);
}

RawScoredTrade* StrategyOptimizerScorer::scoreSingle(
    const std::shared_ptr<Trade>& trade,
    double underlyingPrice)
{
    double sd = calculateOverallImpliedSd(*trade, underlyingPrice);
    StandardDeviationPrices sdPrices(underlyingPrice, sd);
    auto plByPrice = calcProfitLossByPrice(*trade, sdPrices);

    bool anyProfit = false;
    for (const auto& [_, pl] : plByPrice) {
        if (pl >= m_minProfitAmount) { anyProfit = true; break; }
    }
    if (!anyProfit) return nullptr;

    double prob = successProbability(
        plByPrice, underlyingPrice, sd);
    if (prob < m_minProbability) return nullptr;

    int dte = trade->getSells().empty()
        ? trade->getBuys().front().dte
        : trade->getSells().front().dte;
    double annual = annualReturnScore(
        dte, prob, plByPrice, sdPrices, *trade);
    if (annual < m_minAnnualReturn) return nullptr;

    double ppScore = scoreByPricePoints(
        plByPrice, underlyingPrice, sd);
    auto mpl = calcMaxProfitLoss(plByPrice, sdPrices);
    auto deltas = calcDeltas(*trade);
    int ht = hundredTrades(plByPrice, prob);

    Score s;
    s.pricePointScore = ppScore;
    s.numProfitablePointsScore =
        scoreByNumProfitablePoints(plByPrice, sdPrices);
    s.probability = prob;
    s.maxLossRatio = mpl.score;
    s.annualizedReturn = annual;
    s.deltaScore = deltas.deltaScore;
    s.hundredTradesScore = ht;

    auto* result = new RawScoredTrade();
    result->score = s;
    result->plByPrice = plByPrice;
    result->sdPrices = sdPrices;
    result->maxProfitLoss = mpl;
    result->tradeDelta = deltas.tradeDelta;
    result->trade = trade;
    return result;
}

double StrategyOptimizerScorer::calculateOverallImpliedSd(
    const Trade& trade, double underlyingPrice) const
{
    double sum = 0.0;
    auto allLegs = trade.getSells();
    auto buys = trade.getBuys();
    allLegs.insert(allLegs.end(), buys.begin(), buys.end());
    for (const auto& leg : allLegs) {
        sum += calculateImpliedSd(leg, underlyingPrice);
    }
    return sum / static_cast<double>(allLegs.size());
}

double StrategyOptimizerScorer::scoreByPricePoints(
    const std::map<int, double>& plByPrice,
    double underlyingPrice, double sd) const
{
    double total = 0.0;
    for (const auto& [price, pl] : plByPrice) {
        double diff = std::abs(underlyingPrice - price);
        double sdMult = (diff < sd)
            ? 2.0 + sd / std::max(diff, 1.0)
            : 1.0 + sd / diff;
        total += pl * sdMult;
    }
    return total;
}

StrategyOptimizerScorer::Deltas
StrategyOptimizerScorer::calcDeltas(const Trade& trade) const {
    double sum = 0.0;
    for (const auto& o : trade.getSells()) sum += o.delta;
    for (const auto& o : trade.getBuys()) sum += o.delta;

    double buyDelta = 0.0, sellDelta = 0.0;
    for (const auto& o : trade.getBuys())
        buyDelta += std::abs(o.delta);
    for (const auto& o : trade.getSells())
        sellDelta += std::abs(o.delta);

    return { sum, buyDelta - sellDelta };
}

int StrategyOptimizerScorer::hundredTrades(
    const std::map<int, double>& plByPrice,
    double probability) const
{
    double maxProfit = -1e18;
    double maxLoss = 1e18;
    for (const auto& [_, pl] : plByPrice) {
        if (pl > maxProfit) maxProfit = pl;
        if (pl < maxLoss) maxLoss = pl;
    }

    int numMaxProfit = 0, numProfitable = 0;
    int numMaxLoss = 0, numLosses = 0;
    for (const auto& [_, pl] : plByPrice) {
        if (std::abs(pl - maxProfit) < 0.01) ++numMaxProfit;
        if (pl > m_minProfitAmount) ++numProfitable;
        if (std::abs(pl - maxLoss) < 0.01) ++numMaxLoss;
        if (pl < 0) ++numLosses;
    }

    double targetProfit = (numMaxProfit > numProfitable / 2)
        ? maxProfit : 0.0;
    if (targetProfit == 0.0 && numProfitable > 0) {
        double sum = 0.0;
        for (const auto& [_, pl] : plByPrice) {
            if (pl > m_minProfitAmount) sum += pl;
        }
        targetProfit = sum / numProfitable;
    }

    double typicalLoss = (numMaxLoss > numLosses / 2)
        ? maxLoss : 0.0;
    if (typicalLoss == 0.0 && numLosses > 0) {
        double sum = 0.0;
        int cnt = 0;
        for (const auto& [_, pl] : plByPrice) {
            if (pl < m_minProfitAmount) { sum += pl; ++cnt; }
        }
        typicalLoss = (cnt > 0) ? sum / cnt : 0.0;
    }

    int numLosing = 100 - static_cast<int>(probability);
    double losses = numLosing * typicalLoss;
    double wins = static_cast<int>(probability) * targetProfit;
    return static_cast<int>((losses + wins) * 100.0);
}

std::vector<ScoredTrade> StrategyOptimizerScorer::normalize(
    const std::vector<RawScoredTrade>& raw)
{
    auto result = Normalizer::normalize(raw, m_weigher);
    std::sort(result.begin(), result.end(),
        [](const ScoredTrade& a, const ScoredTrade& b) {
            return a.score > b.score;
        });

    std::vector<ScoredTrade> filtered;
    for (auto& st : result) {
        if (st.score <= 0) continue;
        filtered.push_back(std::move(st));
        if (filtered.size() >= MAX_HIGH_SCORES) break;
    }
    return filtered;
}
