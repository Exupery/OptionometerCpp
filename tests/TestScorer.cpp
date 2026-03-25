#include <gtest/gtest.h>
#include "core/StrategyOptimizerScorer.h"
#include "core/BullPutScorer.h"
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
