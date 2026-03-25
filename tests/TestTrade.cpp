#include <gtest/gtest.h>
#include "models/Trade.h"

namespace {

Option makeCall(double strike, double bid, double ask) {
    Option o;
    o.strike = strike;
    o.side = Side::Call;
    o.bid = bid;
    o.ask = ask;
    o.dte = 30;
    o.expiry = 1000000;
    return o;
}

Option makePut(double strike, double bid, double ask) {
    Option o;
    o.strike = strike;
    o.side = Side::Put;
    o.bid = bid;
    o.ask = ask;
    o.dte = 30;
    o.expiry = 1000000;
    return o;
}

class TradeTest : public ::testing::Test {
protected:
    void SetUp() override {
        Trade::setCommissionPerShare(0.005);
    }
};

TEST_F(TradeTest, CallBuyProfitable) {
    Trade t({makeCall(100, 2.0, 2.5)}, {});
    // target=110: 110 - 100 - 2.505 = 7.495
    double pl = t.profitLossAtPrice(110.0);
    EXPECT_NEAR(pl, 7.495, 0.001);
}

TEST_F(TradeTest, CallBuyUnprofitable) {
    Trade t({makeCall(100, 2.0, 2.5)}, {});
    // target=95: -(2.5 + 0.005) = -2.505
    double pl = t.profitLossAtPrice(95.0);
    EXPECT_NEAR(pl, -2.505, 0.001);
}

TEST_F(TradeTest, CallSellProfitable) {
    Trade t({}, {makeCall(100, 2.0, 2.5)});
    // target=95: 2.0 - 0.005 = 1.995
    double pl = t.profitLossAtPrice(95.0);
    EXPECT_NEAR(pl, 1.995, 0.001);
}

TEST_F(TradeTest, CallSellUnprofitable) {
    Trade t({}, {makeCall(100, 2.0, 2.5)});
    // target=110: -(110-100) + 1.995 = -8.005
    double pl = t.profitLossAtPrice(110.0);
    EXPECT_NEAR(pl, -8.005, 0.001);
}

TEST_F(TradeTest, PutBuyProfitable) {
    Trade t({makePut(100, 2.0, 2.5)}, {});
    // target=90: 100 - 90 - 2.505 = 7.495
    double pl = t.profitLossAtPrice(90.0);
    EXPECT_NEAR(pl, 7.495, 0.001);
}

TEST_F(TradeTest, PutBuyUnprofitable) {
    Trade t({makePut(100, 2.0, 2.5)}, {});
    // target=105: -(2.5 + 0.005) = -2.505
    double pl = t.profitLossAtPrice(105.0);
    EXPECT_NEAR(pl, -2.505, 0.001);
}

TEST_F(TradeTest, PutSellProfitable) {
    Trade t({}, {makePut(100, 2.0, 2.5)});
    // target=105: 2.0 - 0.005 = 1.995
    double pl = t.profitLossAtPrice(105.0);
    EXPECT_NEAR(pl, 1.995, 0.001);
}

TEST_F(TradeTest, PutSellUnprofitable) {
    Trade t({}, {makePut(100, 2.0, 2.5)});
    // target=90: -(100-90) + 1.995 = -8.005
    double pl = t.profitLossAtPrice(90.0);
    EXPECT_NEAR(pl, -8.005, 0.001);
}

TEST_F(TradeTest, BullPutSpread) {
    // Buy lower put, sell higher put
    Trade t({makePut(95, 1.0, 1.5)},
            {makePut(100, 3.0, 3.5)});
    // Above both strikes: credit = 2.995 - 1.505 = 1.49
    double pl = t.profitLossAtPrice(105.0);
    EXPECT_NEAR(pl, 1.49, 0.01);
}

TEST_F(TradeTest, RequiredMarginSpread) {
    Trade t({makeCall(100, 2.0, 2.5)},
            {makeCall(110, 4.0, 4.5)});
    double margin = t.requiredMargin();
    EXPECT_GT(margin, 0.0);
}

TEST_F(TradeTest, RequiredMarginBuyOnly) {
    Trade t({makeCall(100, 2.0, 2.5)}, {});
    // buy-only: ask * 100 = 2.5 * 100 = 250
    EXPECT_NEAR(t.requiredMargin(), 250.0, 0.01);
}

TEST_F(TradeTest, ToString) {
    Trade t({makeCall(100, 2.0, 2.5)},
            {makePut(95, 3.0, 3.5)});
    QString str = t.toString();
    EXPECT_TRUE(str.contains("CALL"));
    EXPECT_TRUE(str.contains("PUT"));
}

} // namespace
