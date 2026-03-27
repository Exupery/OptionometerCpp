#include "NakedPutScorer.h"
#include "MathUtils.h"
#include "Normalizer.h"
#include "services/MarketDataImporter.h"
#include <QDate>
#include <algorithm>
#include <cmath>

static constexpr int MAX_HIGH_SCORES = 100;
static int s_scoreSingleLogCount = 0;

NakedPutScorer::NakedPutScorer(
    double minProbability, double minProfitAmount,
    double minAnnualReturn, int maxMargin,
    const Weigher& weigher, int maxLossProbability)
    : Scorer(minProbability, minProfitAmount, minAnnualReturn)
    , m_maxMargin(maxMargin)
    , m_weigher(weigher)
    , m_maxLossProb(maxLossProbability / 100.0)
{}

std::vector<ScoredBullPut> NakedPutScorer::score(
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

double NakedPutScorer::calcMarginPerContract(
    const Option& sellPut, double underlyingPrice)
{
    double premium = sellPut.bid * 100.0;
    double otmAmount = std::max(0.0,
        (underlyingPrice - sellPut.strike) * 100.0);

    double rule20 = 0.20 * underlyingPrice * 100.0
        - otmAmount + premium;
    double rule10 = 0.10 * sellPut.strike * 100.0 + premium;
    double ruleMin = 250.0;

    return std::max({rule20, rule10, ruleMin});
}

RawScoredBullPut* NakedPutScorer::scoreSingle(
    const std::shared_ptr<Trade>& trade,
    double underlyingPrice)
{
    bool shouldLog = (s_scoreSingleLogCount < 3);

    if (!isValidNakedPut(*trade)) {
        if (shouldLog) {
            ++s_scoreSingleLogCount;
            MarketDataImporter::appendToLog(
                "    scoreSingle REJECTED: invalid naked put - "
                + trade->toString());
        }
        return nullptr;
    }

    const auto& sellOpt = trade->getSells().front();
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
                "    scoreSingle REJECTED: no profit >= "
                + QString::number(m_minProfitAmount)
                + " maxPl=" + QString::number(maxPl, 'f', 4)
                + " sellStrike=" + QString::number(sellOpt.strike, 'f', 1)
                + " sellBid=" + QString::number(sellOpt.bid, 'f', 4));
        }
        return nullptr;
    }

    double prob = successProbability(
        plByPrice, underlyingPrice, sd, sellOpt.dte);
    if (prob < m_minProbability) {
        if (shouldLog) {
            ++s_scoreSingleLogCount;
            MarketDataImporter::appendToLog(
                "    scoreSingle REJECTED: prob="
                + QString::number(prob, 'f', 2)
                + " < minProb=" + QString::number(m_minProbability));
        }
        return nullptr;
    }

    double marginPerContract = calcMarginPerContract(
        sellOpt, underlyingPrice);
    int numContracts = static_cast<int>(
        m_maxMargin / marginPerContract);
    if (numContracts < 1) {
        if (shouldLog) {
            ++s_scoreSingleLogCount;
            MarketDataImporter::appendToLog(
                "    scoreSingle REJECTED: margin per contract="
                + QString::number(marginPerContract, 'f', 2)
                + " exceeds maxMargin=" + QString::number(m_maxMargin));
        }
        return nullptr;
    }

    double sigmaRootT = sd / underlyingPrice;
    double T = static_cast<double>(sellOpt.dte) / 365.0;

    auto perContractPl = calcNakedPutMaxProfitLoss(
        *trade, prob, plByPrice, underlyingPrice, sigmaRootT, T);

    MaxProfitLoss tradePl;
    tradePl.maxProfitToMaxLossRatio =
        perContractPl.maxProfitToMaxLossRatio;
    tradePl.numMaxProfit = perContractPl.numMaxProfit;
    tradePl.maxProfit = perContractPl.maxProfit * numContracts * 100.0;
    tradePl.numMaxLoss = perContractPl.numMaxLoss;
    tradePl.maxLoss = perContractPl.maxLoss * numContracts * 100.0;
    tradePl.score = perContractPl.score;

    double annualReturn = nakedPutAnnualReturn(
        *trade, tradePl, prob, numContracts, marginPerContract,
        underlyingPrice, sigmaRootT, T);
    if (annualReturn < m_minAnnualReturn) {
        if (shouldLog) {
            ++s_scoreSingleLogCount;
            MarketDataImporter::appendToLog(
                "    scoreSingle REJECTED: annual="
                + QString::number(annualReturn, 'f', 2)
                + " < minAnnual=" + QString::number(m_minAnnualReturn));
        }
        return nullptr;
    }

    Score s;
    s.pricePointScore =
        underlyingPrice - sellOpt.strike;
    s.numProfitablePointsScore = static_cast<double>(
        std::count_if(plByPrice.begin(), plByPrice.end(),
            [](const auto& p) { return p.second > 0; }));
    s.probability = prob;
    s.maxLossRatio = tradePl.maxProfitToMaxLossRatio;
    s.annualizedReturn = annualReturn;
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

bool NakedPutScorer::isValidNakedPut(const Trade& trade) const {
    return trade.getSells().size() == 1
        && trade.getBuys().empty()
        && trade.getSells().front().side == Side::Put;
}

int NakedPutScorer::weekdaysInDte(int dte) {
    QDate today = QDate::currentDate();
    int weekdays = 0;
    for (int d = 1; d <= dte; ++d) {
        int dow = today.addDays(d).dayOfWeek();
        if (dow >= 1 && dow <= 5) ++weekdays;
    }
    return std::max(weekdays, 1);
}

MaxProfitLoss NakedPutScorer::calcNakedPutMaxProfitLoss(
    const Trade& trade, double probability,
    const std::map<int, double>& plByPrice,
    double underlyingPrice,
    double sigmaRootT, double T) const
{
    const auto& sellPut = trade.getSells().front();
    double credit = sellPut.bid;

    // Max loss at the configured probability price (VaR approach):
    // the price the underlying has only that % chance of falling to
    double worstPrice = MathUtils::lognormalInverseCdf(
        m_maxLossProb, underlyingPrice, sigmaRootT, T);
    // Ensure worst price is always below the strike so max loss
    // is never zero — even high-probability trades can lose
    worstPrice = std::min(worstPrice, sellPut.strike - 1.0);
    double lossAtWorst = std::max(0.0,
        sellPut.strike - worstPrice) - credit;
    double maxLoss = std::max(lossAtWorst, 0.0);
    double ratio = (maxLoss > 0.0)
        ? credit / maxLoss : credit;

    int numProfit = 0, numLoss = 0;
    for (const auto& [_, pl] : plByPrice) {
        if (pl > 0) ++numProfit; else ++numLoss;
    }

    double score = ((ratio * 100.0) + probability)
        * (static_cast<double>(numProfit) / plByPrice.size());

    return { ratio, numProfit, credit,
             numLoss, -maxLoss, score };
}

double NakedPutScorer::nakedPutAnnualReturn(
    const Trade& trade, const MaxProfitLoss& tradePl,
    double probability, int numContracts,
    double marginPerContract,
    double underlyingPrice,
    double sigmaRootT, double T) const
{
    const auto& sellPut = trade.getSells().front();
    double credit = sellPut.bid;
    double breakevenPrice = sellPut.strike - credit;

    // Typical loss at a probability halfway between the max loss
    // probability and the breakeven probability
    double breakevenProb = MathUtils::lognormalCdf(
        breakevenPrice, underlyingPrice, sigmaRootT, T);
    double typicalLossProb = (breakevenProb + m_maxLossProb) / 2.0;
    double typicalLossPrice = MathUtils::lognormalInverseCdf(
        typicalLossProb, underlyingPrice, sigmaRootT, T);
    double typicalLossPerShare = std::max(0.0,
        sellPut.strike - typicalLossPrice) - credit;
    double typicalLoss = std::max(
        -typicalLossPerShare * numContracts * 100.0,
        tradePl.maxLoss);

    int tradesPerYear = std::max(
        365 / std::max(sellPut.dte, 7), 1);
    double profit = tradesPerYear
        * (probability / 100.0) * tradePl.maxProfit;
    double loss = tradesPerYear
        * ((100.0 - probability) / 100.0) * typicalLoss;
    return (profit + loss) / m_maxMargin * 100.0;
}

std::vector<ScoredBullPut> NakedPutScorer::normalize(
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
