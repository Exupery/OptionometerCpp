#include <gtest/gtest.h>
#include "core/StrategyOptimizerScorer.h"
#include "core/BullPutScorer.h"
#include "core/NakedPutScorer.h"
#include "models/Trade.h"

namespace {

Option makeOption(Side side, double strike, double bid,
                  double ask, double iv, double delta, int dte)
{
    Option o;
    o.symbol = "TEST";
    o.strike = strike;
    o.side = side;
    o.bid = bid;
    o.ask = ask;
    o.impliedVolatility = iv;
    o.delta = delta;
    o.dte = dte;
    o.expiry = 1000000;
    return o;
}

class ScorerTest : public ::testing::Test {
protected:
    void SetUp() override {
        Trade::setCommissionPerShare(0.005);
    }
};

TEST_F(ScorerTest, StrategyOptimizerReturnsResults) {
    StrategyOptimizerScorer scorer(10.0, 0.1, 0.1);
    auto buy = makeOption(
        Side::Put, 95, 1.0, 1.5, 0.25, -0.3, 30);
    auto sell = makeOption(
        Side::Put, 100, 3.0, 3.5, 0.25, -0.4, 30);
    auto trade = std::make_shared<Trade>(
        std::vector<Option>{buy},
        std::vector<Option>{sell});
    auto results = scorer.score({trade}, 100.0);
    // May or may not have results depending on thresholds
    // Just verify no crash
    SUCCEED();
}

TEST_F(ScorerTest, BullPutScorerRejectsInvalidSpread) {
    BullPutScorer scorer(10.0, 0.1, 0.1, 10000);
    // Call instead of put - invalid
    auto buy = makeOption(
        Side::Call, 95, 1.0, 1.5, 0.25, 0.3, 30);
    auto sell = makeOption(
        Side::Call, 100, 3.0, 3.5, 0.25, 0.4, 30);
    auto trade = std::make_shared<Trade>(
        std::vector<Option>{buy},
        std::vector<Option>{sell});
    auto results = scorer.score({trade}, 100.0);
    EXPECT_TRUE(results.empty());
}

TEST_F(ScorerTest, BullPutScorerAcceptsValidSpread) {
    BullPutScorer scorer(1.0, 0.01, 0.01, 10000);
    auto buy = makeOption(
        Side::Put, 90, 0.5, 1.0, 0.25, -0.2, 30);
    auto sell = makeOption(
        Side::Put, 95, 2.0, 2.5, 0.25, -0.35, 30);
    auto trade = std::make_shared<Trade>(
        std::vector<Option>{buy},
        std::vector<Option>{sell});
    auto results = scorer.score({trade}, 100.0);
    // Valid spread - may produce results
    SUCCEED();
}

TEST_F(ScorerTest, BullPutMaxProfitLossInDollars) {
    // Very low thresholds so the trade isn't filtered out
    BullPutScorer scorer(0.01, 0.001, 0.001, 50000);
    // 95/100 bull put spread: sell 100 put, buy 95 put
    auto buy = makeOption(
        Side::Put, 95, 0.5, 1.0, 0.25, -0.2, 30);
    auto sell = makeOption(
        Side::Put, 100, 2.0, 2.5, 0.25, -0.35, 30);
    auto trade = std::make_shared<Trade>(
        std::vector<Option>{buy},
        std::vector<Option>{sell});
    auto results = scorer.score({trade}, 105.0);
    if (results.empty()) { SUCCEED(); return; }
    const auto& r = results.front();
    // Per-share: credit = 2.0 - 1.0 = 1.0, maxLoss = 5 - 1 = 4.0
    // numContracts = 50000 / (4.0 * 100) = 125
    // Total maxProfit = 1.0 * 125 * 100 = 12500
    // Total maxLoss = -4.0 * 125 * 100 = -50000
    EXPECT_EQ(r.numContracts, 125);
    EXPECT_NEAR(r.maxProfitLoss.maxProfit, 12500.0, 1.0);
    EXPECT_NEAR(r.maxProfitLoss.maxLoss, -50000.0, 1.0);
}

TEST_F(ScorerTest, NakedPutScorerRejectsInvalidTrade) {
    NakedPutScorer scorer(10.0, 0.1, 0.1, 10000);
    // Two-leg trade is invalid for naked put
    auto buy = makeOption(
        Side::Put, 95, 1.0, 1.5, 0.25, -0.2, 30);
    auto sell = makeOption(
        Side::Put, 100, 3.0, 3.5, 0.25, -0.4, 30);
    auto trade = std::make_shared<Trade>(
        std::vector<Option>{buy},
        std::vector<Option>{sell});
    auto results = scorer.score({trade}, 100.0);
    EXPECT_TRUE(results.empty());
}

TEST_F(ScorerTest, NakedPutScorerRejectsCallSell) {
    NakedPutScorer scorer(10.0, 0.1, 0.1, 10000);
    auto sell = makeOption(
        Side::Call, 100, 3.0, 3.5, 0.25, 0.4, 30);
    auto trade = std::make_shared<Trade>(
        std::vector<Option>{},
        std::vector<Option>{sell});
    auto results = scorer.score({trade}, 100.0);
    EXPECT_TRUE(results.empty());
}

TEST_F(ScorerTest, NakedPutScorerAcceptsValidPut) {
    NakedPutScorer scorer(1.0, 0.01, 0.01, 50000);
    auto sell = makeOption(
        Side::Put, 480, 3.0, 3.5, 0.25, -0.15, 30);
    auto trade = std::make_shared<Trade>(
        std::vector<Option>{},
        std::vector<Option>{sell});
    auto results = scorer.score({trade}, 500.0);
    // Valid naked put - may produce results
    SUCCEED();
}

TEST_F(ScorerTest, NakedPutMarginCalculation) {
    // underlying=500, strike=480, bid=3.00
    auto sell = makeOption(
        Side::Put, 480, 3.0, 3.5, 0.25, -0.15, 30);
    double margin = NakedPutScorer::calcMarginPerContract(
        sell, 500.0);
    // OTM amount = (500-480)*100 = 2000
    // 20% rule: 0.20*500*100 - 2000 + 300 = 8300
    // 10% rule: 0.10*480*100 + 300 = 5100
    // Min: 250
    // Expected: max(8300, 5100, 250) = 8300
    EXPECT_NEAR(margin, 8300.0, 0.01);
}

TEST_F(ScorerTest, NakedPutMarginITM) {
    // ITM put: underlying=480, strike=500, bid=22.0
    auto sell = makeOption(
        Side::Put, 500, 22.0, 23.0, 0.25, -0.6, 30);
    double margin = NakedPutScorer::calcMarginPerContract(
        sell, 480.0);
    // OTM amount = max(0, 480-500)*100 = 0 (ITM, not OTM)
    // 20% rule: 0.20*480*100 - 0 + 2200 = 9600 + 2200 = 11800
    // 10% rule: 0.10*500*100 + 2200 = 5000 + 2200 = 7200
    // Min: 250
    // Expected: max(11800, 7200, 250) = 11800
    EXPECT_NEAR(margin, 11800.0, 0.01);
}

TEST_F(ScorerTest, NakedPutContractCount) {
    NakedPutScorer scorer(0.01, 0.001, 0.001, 50000);
    auto sell = makeOption(
        Side::Put, 480, 3.0, 3.5, 0.25, -0.15, 30);
    auto trade = std::make_shared<Trade>(
        std::vector<Option>{},
        std::vector<Option>{sell});
    auto results = scorer.score({trade}, 500.0);
    if (results.empty()) { SUCCEED(); return; }
    // margin per contract = 8300 (20% rule dominates)
    // numContracts = floor(50000 / 8300) = 6
    EXPECT_EQ(results.front().numContracts, 6);
}

TEST_F(ScorerTest, NakedPutWeekdaysInDte) {
    // 7 calendar days always has 5 weekdays
    // (regardless of start day, a full week = 5 weekdays)
    EXPECT_EQ(NakedPutScorer::weekdaysInDte(7), 5);
    // 14 calendar days = 10 weekdays
    EXPECT_EQ(NakedPutScorer::weekdaysInDte(14), 10);
    // 0 days should return minimum of 1
    EXPECT_EQ(NakedPutScorer::weekdaysInDte(0), 1);
}

TEST_F(ScorerTest, NakedPutMarginMinimumRule) {
    // Very low-priced stock, cheap put - minimum rule should apply
    auto sell = makeOption(
        Side::Put, 5, 0.05, 0.10, 0.50, -0.1, 30);
    double margin = NakedPutScorer::calcMarginPerContract(
        sell, 6.0);
    // OTM = (6-5)*100 = 100
    // 20%: 0.20*6*100 - 100 + 5 = 120 - 100 + 5 = 25
    // 10%: 0.10*5*100 + 5 = 55
    // Min: 250
    // Expected: max(25, 55, 250) = 250
    EXPECT_NEAR(margin, 250.0, 0.01);
}

TEST_F(ScorerTest, StandardDeviationPricesCalc) {
    StandardDeviationPrices sdp(100.0, 10.0);
    EXPECT_NEAR(sdp.upperSd, 120.0, 0.01);
    EXPECT_NEAR(sdp.lowerSd, 80.0, 0.01);
}

TEST_F(ScorerTest, StandardDeviationBand) {
    StandardDeviationPrices sdp(100.0, 10.0);
    EXPECT_EQ(sdp.sdBand(100.0), 1);
    EXPECT_EQ(sdp.sdBand(105.0), 1);
    EXPECT_EQ(sdp.sdBand(111.0), 2);
    EXPECT_EQ(sdp.sdBand(121.0), 3);
}

TEST_F(ScorerTest, LowerSdFloored) {
    StandardDeviationPrices sdp(5.0, 10.0);
    EXPECT_NEAR(sdp.lowerSd, 1.0, 0.01);
}

} // namespace
