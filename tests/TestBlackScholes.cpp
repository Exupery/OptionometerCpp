#include <gtest/gtest.h>
#include "core/BlackScholes.h"
#include "models/Option.h"
#include "models/Trade.h"
#include <cmath>
#include <vector>

class BlackScholesTest : public ::testing::Test {
protected:
    void SetUp() override {
        Trade::setCommissionPerShare(0.005);
    }
};

TEST_F(BlackScholesTest, CallPriceAtExpiration) {
    // At expiration (T=0), call value = max(S-K, 0)
    EXPECT_NEAR(BlackScholes::callPrice(110, 100, 0, 0.3),
                10.0, 0.001);
    EXPECT_NEAR(BlackScholes::callPrice(90, 100, 0, 0.3),
                0.0, 0.001);
}

TEST_F(BlackScholesTest, PutPriceAtExpiration) {
    // At expiration (T=0), put value = max(K-S, 0)
    EXPECT_NEAR(BlackScholes::putPrice(90, 100, 0, 0.3),
                10.0, 0.001);
    EXPECT_NEAR(BlackScholes::putPrice(110, 100, 0, 0.3),
                0.0, 0.001);
}

TEST_F(BlackScholesTest, CallPricePositiveTimeValue) {
    // With time remaining, call should be worth more than
    // intrinsic value
    double atm = BlackScholes::callPrice(100, 100, 0.25, 0.3);
    EXPECT_GT(atm, 0.0);

    double itm = BlackScholes::callPrice(110, 100, 0.25, 0.3);
    EXPECT_GT(itm, 10.0);
}

TEST_F(BlackScholesTest, PutPricePositiveTimeValue) {
    double atm = BlackScholes::putPrice(100, 100, 0.25, 0.3);
    EXPECT_GT(atm, 0.0);

    double itm = BlackScholes::putPrice(90, 100, 0.25, 0.3);
    EXPECT_GT(itm, 10.0);
}

TEST_F(BlackScholesTest, PutCallParity) {
    // C - P = S - K * exp(-rT)
    double S = 100, K = 100, T = 0.5, sigma = 0.25, r = 0.05;
    double call = BlackScholes::callPrice(S, K, T, sigma, r);
    double put = BlackScholes::putPrice(S, K, T, sigma, r);
    double parity = S - K * std::exp(-r * T);
    EXPECT_NEAR(call - put, parity, 0.001);
}

TEST_F(BlackScholesTest, CallPriceIncreasesWithVolatility) {
    double low = BlackScholes::callPrice(100, 100, 0.25, 0.15);
    double high = BlackScholes::callPrice(100, 100, 0.25, 0.45);
    EXPECT_GT(high, low);
}

TEST_F(BlackScholesTest, CallPriceIncreasesWithTime) {
    double short_t = BlackScholes::callPrice(100, 100, 0.1, 0.3);
    double long_t = BlackScholes::callPrice(100, 100, 1.0, 0.3);
    EXPECT_GT(long_t, short_t);
}

TEST_F(BlackScholesTest, ZeroUnderlyingPrice) {
    EXPECT_NEAR(BlackScholes::callPrice(0, 100, 0.25, 0.3),
                0.0, 0.001);
    double putVal = BlackScholes::putPrice(0, 100, 0.25, 0.3);
    double expected = 100.0 * std::exp(-0.05 * 0.25);
    EXPECT_NEAR(putVal, expected, 0.001);
}

TEST_F(BlackScholesTest, CurrentPlBullPutSpread) {
    // Sell put at 100, buy put at 95
    Option sellPut;
    sellPut.strike = 100;
    sellPut.side = Side::Put;
    sellPut.bid = 3.00;
    sellPut.ask = 3.20;
    sellPut.impliedVolatility = 0.25;
    sellPut.dte = 30;

    Option buyPut;
    buyPut.strike = 95;
    buyPut.side = Side::Put;
    buyPut.bid = 1.00;
    buyPut.ask = 1.20;
    buyPut.impliedVolatility = 0.28;
    buyPut.dte = 30;

    Trade trade({buyPut}, {sellPut});

    // At underlying = 120 (both OTM), current P/L should be
    // close to max profit (net credit received)
    double plHigh = BlackScholes::currentPlAtPrice(trade, 120, 30);
    double credit = (sellPut.bid - Trade::commissionPerShare())
        - (buyPut.ask + Trade::commissionPerShare());
    EXPECT_GT(plHigh, 0.0);
    EXPECT_LT(plHigh, credit + 0.01);

    // At underlying = 80 (both deep ITM), should be close to
    // max loss
    double plLow = BlackScholes::currentPlAtPrice(trade, 80, 30);
    EXPECT_LT(plLow, 0.0);
}

TEST_F(BlackScholesTest, HalfDteRoundingUp) {
    // Half-DTE calculation: (dte + 1) / 2
    // Even DTE: 10 -> 5
    EXPECT_EQ((10 + 1) / 2, 5);
    // Odd DTE: 5 -> 3
    EXPECT_EQ((5 + 1) / 2, 3);
    // Odd DTE: 7 -> 4
    EXPECT_EQ((7 + 1) / 2, 4);
    // Even DTE: 2 -> 1
    EXPECT_EQ((2 + 1) / 2, 1);
    // Odd DTE: 3 -> 2
    EXPECT_EQ((3 + 1) / 2, 2);
    // Odd DTE: 1 -> 1 (but DTE=1 is disabled, so not used)
    EXPECT_EQ((1 + 1) / 2, 1);
}

TEST_F(BlackScholesTest, HalfDtePlBetweenCurrentAndExpiration) {
    // Half-DTE P/L should fall between current P/L and expiration
    // P/L in terms of time decay
    Option sellPut;
    sellPut.strike = 100;
    sellPut.side = Side::Put;
    sellPut.bid = 3.00;
    sellPut.ask = 3.20;
    sellPut.impliedVolatility = 0.25;
    sellPut.dte = 30;

    Option buyPut;
    buyPut.strike = 95;
    buyPut.side = Side::Put;
    buyPut.bid = 1.00;
    buyPut.ask = 1.20;
    buyPut.impliedVolatility = 0.28;
    buyPut.dte = 30;

    Trade trade({buyPut}, {sellPut});
    int halfDte = (30 + 1) / 2;  // 15

    // At underlying = 120 (OTM), half-DTE P/L should be between
    // current and expiration values (closer to expiration = more
    // profit for short puts)
    double currentPl = BlackScholes::currentPlAtPrice(trade, 120, 30);
    double halfPl = BlackScholes::currentPlAtPrice(trade, 120, halfDte);
    double expirationPl = trade.profitLossAtPrice(120);

    // For a credit spread far OTM, as time passes profit increases
    EXPECT_GT(halfPl, currentPl);
    EXPECT_LT(halfPl, expirationPl + 0.01);
}

TEST_F(BlackScholesTest, CurrentPlAtExpirationMatchesTradePl) {
    // At DTE=0, Black-Scholes current P/L should match
    // Trade::profitLossAtPrice
    Option sellPut;
    sellPut.strike = 100;
    sellPut.side = Side::Put;
    sellPut.bid = 3.00;
    sellPut.ask = 3.20;
    sellPut.impliedVolatility = 0.25;
    sellPut.dte = 0;

    Option buyPut;
    buyPut.strike = 95;
    buyPut.side = Side::Put;
    buyPut.bid = 1.00;
    buyPut.ask = 1.20;
    buyPut.impliedVolatility = 0.28;
    buyPut.dte = 0;

    Trade trade({buyPut}, {sellPut});

    for (double price : {80.0, 90.0, 95.0, 97.5, 100.0, 110.0}) {
        double bsPl = BlackScholes::currentPlAtPrice(
            trade, price, 0);
        double tradePl = trade.profitLossAtPrice(price);
        EXPECT_NEAR(bsPl, tradePl, 0.01)
            << "Price: " << price;
    }
}
