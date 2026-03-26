#include "Normalizer.h"
#include <algorithm>
#include <numeric>

namespace Normalizer {

std::map<int, double> normalizeScores(
    const std::vector<std::pair<int, double>>& indexedScores,
    bool ignoreNegative)
{
    if (indexedScores.size() <= 1) {
        std::map<int, double> result;
        for (const auto& [idx, _] : indexedScores) {
            result[idx] = (indexedScores.size() == 1) ? 100.0 : 0.0;
        }
        return result;
    }

    auto sorted = indexedScores;
    std::sort(sorted.begin(), sorted.end(),
        [](const auto& a, const auto& b) {
            return a.second < b.second;
        });

    double interval = 100.0
        / (static_cast<double>(sorted.size()) - 1.0);
    std::map<int, double> result;
    for (size_t i = 0; i < sorted.size(); ++i) {
        double pct = (ignoreNegative && sorted[i].second <= 0.0)
            ? 0.0
            : i * interval;
        result[sorted[i].first] = pct;
    }
    return result;
}

std::vector<ScoredTrade> normalize(
    const std::vector<RawScoredTrade>& rawScored,
    const Weigher& weigher)
{
    if (rawScored.empty()) return {};

    auto indexed = [&](auto extractor) {
        std::vector<std::pair<int, double>> v;
        for (int i = 0; i < static_cast<int>(rawScored.size()); ++i)
            v.emplace_back(i, extractor(rawScored[i]));
        return v;
    };

    auto ppScores = normalizeScores(
        indexed([](const auto& r) {
            return r.score.pricePointScore; }));
    auto npScores = normalizeScores(
        indexed([](const auto& r) {
            return r.score.numProfitablePointsScore; }));
    auto probScores = normalizeScores(
        indexed([](const auto& r) {
            return r.score.probability; }));
    auto mplScores = normalizeScores(
        indexed([](const auto& r) {
            return r.score.maxLossRatio; }));
    auto arScores = normalizeScores(
        indexed([](const auto& r) {
            return r.score.annualizedReturn; }));
    auto dScores = normalizeScores(
        indexed([](const auto& r) {
            return r.score.deltaScore; }));
    auto htScores = normalizeScores(
        indexed([](const auto& r) {
            return static_cast<double>(
                r.score.hundredTradesScore); }),
        weigher.hundredTradeWeight() <= 1);

    std::vector<ScoredTrade> result;
    result.reserve(rawScored.size());
    for (int i = 0; i < static_cast<int>(rawScored.size()); ++i) {
        const auto& raw = rawScored[i];
        double score = weigher.scoreByWeight(
            ppScores[i], npScores[i], probScores[i],
            mplScores[i], arScores[i], dScores[i], htScores[i]);

        ScoredTrade st;
        st.score = static_cast<int>(score);
        st.plByPrice = raw.plByPrice;
        st.sdPrices = raw.sdPrices;
        st.successProbability = raw.score.probability;
        st.maxProfitLoss = raw.maxProfitLoss;
        st.annualizedReturn = raw.score.annualizedReturn;
        st.tradeDelta = raw.tradeDelta;
        st.hundredTrades = raw.score.hundredTradesScore;
        st.trade = raw.trade;
        result.push_back(std::move(st));
    }
    return result;
}

std::vector<ScoredBullPut> normalizeBullPuts(
    const std::vector<RawScoredBullPut>& rawScored,
    const Weigher& weigher)
{
    if (rawScored.empty()) return {};

    auto indexed = [&](auto extractor) {
        std::vector<std::pair<int, double>> v;
        for (int i = 0; i < static_cast<int>(rawScored.size()); ++i)
            v.emplace_back(i, extractor(rawScored[i]));
        return v;
    };

    auto npScores = normalizeScores(
        indexed([](const auto& r) {
            return r.score.numProfitablePointsScore; }));
    auto probScores = normalizeScores(
        indexed([](const auto& r) {
            return r.score.probability; }));
    auto mplScores = normalizeScores(
        indexed([](const auto& r) {
            return r.score.maxLossRatio; }));
    auto arScores = normalizeScores(
        indexed([](const auto& r) {
            return r.score.annualizedReturn; }));

    std::vector<ScoredBullPut> result;
    result.reserve(rawScored.size());
    for (int i = 0; i < static_cast<int>(rawScored.size()); ++i) {
        const auto& raw = rawScored[i];
        double score = weigher.scoreByWeightBullPut(
            npScores[i], probScores[i], mplScores[i], arScores[i]);

        ScoredBullPut sb;
        sb.score = static_cast<int>(score);
        sb.successProbability = raw.score.probability;
        sb.maxProfitLoss = raw.maxProfitLoss;
        sb.annualizedReturn = raw.score.annualizedReturn;
        sb.sdPrices = raw.sdPrices;
        sb.numContracts = raw.numContracts;
        sb.trade = raw.trade;
        result.push_back(std::move(sb));
    }
    return result;
}

} // namespace Normalizer
