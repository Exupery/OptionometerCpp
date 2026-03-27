#include "BullPutScorer.h"
#include "Normalizer.h"
#include "services/MarketDataImporter.h"
#include <algorithm>
#include <cmath>

static constexpr int MAX_HIGH_SCORES = 100;
static int s_scoreSingleLogCount = 0;

BullPutScorer::BullPutScorer(
    double minProbability, double minProfitAmount,
    double minAnnualReturn, int maxMargin,
    const Weigher& weigher)
    : Scorer(minProbability, minProfitAmount, minAnnualReturn)
    , m_maxMargin(maxMargin)
    , m_weigher(weigher)
{}

std::vector<ScoredBullPut> BullPutScorer::score(
    const std::vector<std::shared_ptr<Trade>>& trades,
    double underlyingPrice)
{
    s_scoreSingleLogCount = 0;
    std::vector<RawScoredBullPut> rawScored;
    for (const auto& trade : trades) {
        auto* raw = scoreSingle(trade, underlyingPrice);
        if (raw) {
            rawScored.push_back(std::move(*raw));
            delete raw;
        }
    }
    return normalize(rawScored);
}

RawScoredBullPut* BullPutScorer::scoreSingle(
    const std::shared_ptr<Trade>& trade,
    double underlyingPrice)
{
    bool shouldLog = (s_scoreSingleLogCount < 3);

    if (!isValidBullPutSpread(*trade)) {
        if (shouldLog) {
            ++s_scoreSingleLogCount;
            MarketDataImporter::appendToLog(
                "    scoreSingle REJECTED: invalid bull put spread - "
                + trade->toString());
        }
        return nullptr;
    }

    const auto& sellOpt = trade->getSells().front();
    const auto& buyOpt = trade->getBuys().front();
    double sd = calculateImpliedSd(sellOpt, underlyingPrice);
    StandardDeviationPrices sdPrices(underlyingPrice, sd);
    auto plByPrice = calcProfitLossByPrice(*trade, sdPrices);

    bool anyProfit = false;
    double maxPl = -1e18;
    for (const auto& [_, pl] : plByPrice) {
        if (pl > maxPl) maxPl = pl;
        if (pl >= m_minProfitAmount) { anyProfit = true; break; }
    }
    if (!anyProfit) {
        if (shouldLog) {
            ++s_scoreSingleLogCount;
            MarketDataImporter::appendToLog(
                "    scoreSingle REJECTED: no profit >= " + QString::number(m_minProfitAmount)
                + " maxPl=" + QString::number(maxPl, 'f', 4)
                + " plByPrice.size=" + QString::number(plByPrice.size())
                + " sd=" + QString::number(sd, 'f', 4)
                + " sellIV=" + QString::number(sellOpt.impliedVolatility, 'f', 4)
                + " sellDte=" + QString::number(sellOpt.dte)
                + " sellStrike=" + QString::number(sellOpt.strike, 'f', 1)
                + " sellBid=" + QString::number(sellOpt.bid, 'f', 4)
                + " buyStrike=" + QString::number(buyOpt.strike, 'f', 1)
                + " buyAsk=" + QString::number(buyOpt.ask, 'f', 4));
        }
        return nullptr;
    }

    double prob = successProbability(
        plByPrice, underlyingPrice, sd, sellOpt.dte);
    if (prob < m_minProbability) {
        if (shouldLog) {
            ++s_scoreSingleLogCount;
            MarketDataImporter::appendToLog(
                "    scoreSingle REJECTED: prob=" + QString::number(prob, 'f', 2)
                + " < minProb=" + QString::number(m_minProbability));
        }
        return nullptr;
    }

    auto perSpreadPl = calcBullPutMaxProfitLoss(
        *trade, prob, plByPrice);
    int numContracts = static_cast<int>(
        m_maxMargin / (std::abs(perSpreadPl.maxLoss) * 100.0));

    MaxProfitLoss tradePl;
    tradePl.maxProfitToMaxLossRatio =
        perSpreadPl.maxProfitToMaxLossRatio;
    tradePl.numMaxProfit = perSpreadPl.numMaxProfit;
    tradePl.maxProfit = perSpreadPl.maxProfit * numContracts * 100.0;
    tradePl.numMaxLoss = perSpreadPl.numMaxLoss;
    tradePl.maxLoss = perSpreadPl.maxLoss * numContracts * 100.0;
    tradePl.score = perSpreadPl.score;

    double annualBullPut = bullPutAnnualReturn(
        *trade, tradePl, prob);
    if (annualBullPut < m_minAnnualReturn) {
        if (shouldLog) {
            ++s_scoreSingleLogCount;
            MarketDataImporter::appendToLog(
                "    scoreSingle REJECTED: annual=" + QString::number(annualBullPut, 'f', 2)
                + " < minAnnual=" + QString::number(m_minAnnualReturn));
        }
        return nullptr;
    }

    Score s;
    s.pricePointScore =
        underlyingPrice - trade->getSells().front().strike;
    s.numProfitablePointsScore = static_cast<double>(
        std::count_if(plByPrice.begin(), plByPrice.end(),
            [](const auto& p) { return p.second > 0; }));
    s.probability = prob;
    s.maxLossRatio = tradePl.maxProfitToMaxLossRatio;
    s.annualizedReturn = annualBullPut;
    s.deltaScore = 0.0;
    s.hundredTradesScore = static_cast<int>(prob);

    auto* result = new RawScoredBullPut();
    result->score = s;
    result->plByPrice = plByPrice;
    result->sdPrices = sdPrices;
    result->maxProfitLoss = tradePl;
    result->numContracts = numContracts;
    result->trade = trade;
    return result;
}

bool BullPutScorer::isValidBullPutSpread(const Trade& trade) const {
    if (trade.getSells().size() != 1 || trade.getBuys().size() != 1)
        return false;
    const auto& sell = trade.getSells().front();
    const auto& buy = trade.getBuys().front();
    if (buy.side != Side::Put || sell.side != Side::Put)
        return false;
    return buy.strike < sell.strike;
}

MaxProfitLoss BullPutScorer::calcBullPutMaxProfitLoss(
    const Trade& trade, double probability,
    const std::map<int, double>& plByPrice) const
{
    const auto& shortPut = trade.getSells().front();
    const auto& longPut = trade.getBuys().front();
    double credit = shortPut.bid - longPut.ask;
    double maxLoss = (shortPut.strike - longPut.strike) - credit;
    double ratio = credit / std::abs(maxLoss);

    int numProfit = 0, numLoss = 0;
    for (const auto& [_, pl] : plByPrice) {
        if (pl > 0) ++numProfit; else ++numLoss;
    }

    double score = ((ratio * 100.0) + probability)
        * (static_cast<double>(numProfit) / plByPrice.size());

    return { ratio, numProfit, credit,
             numLoss, -maxLoss, score };
}

double BullPutScorer::bullPutAnnualReturn(
    const Trade& trade, const MaxProfitLoss& tradePl,
    double probability) const
{
    const auto& shortPut = trade.getSells().front();
    double spread = shortPut.strike - trade.getBuys().front().strike;
    double pct = spread / shortPut.strike;
    double multiplier = (pct < 0.01) ? 0.3 : 0.15;
    double typicalLoss = tradePl.maxLoss * multiplier;
    int tradesPerYear = std::max(
        365 / std::max(shortPut.dte, 7), 1);
    double profit = tradesPerYear
        * (probability / 100.0) * tradePl.maxProfit;
    double loss = tradesPerYear
        * ((100.0 - probability) / 100.0) * typicalLoss;
    return (profit + loss) / m_maxMargin * 100.0;
}

std::vector<ScoredBullPut> BullPutScorer::normalize(
    const std::vector<RawScoredBullPut>& raw)
{
    auto result = Normalizer::normalizeBullPuts(raw, m_weigher);
    std::sort(result.begin(), result.end(),
        [](const ScoredBullPut& a, const ScoredBullPut& b) {
            return a.score > b.score;
        });

    std::vector<ScoredBullPut> filtered;
    for (auto& s : result) {
        if (s.score <= 0) continue;
        filtered.push_back(std::move(s));
        if (filtered.size() >= MAX_HIGH_SCORES) break;
    }
    return filtered;
}
