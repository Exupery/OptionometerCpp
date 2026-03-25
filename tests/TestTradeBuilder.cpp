#include <gtest/gtest.h>
#include "core/TradeBuilder.h"

namespace {

OptionChain makeChain() {
    OptionChain chain;
    chain.underlying = "TEST";
    chain.underlyingPrice = 100.0;
    chain.expiry = 1000000;

    for (int i = 0; i < 5; ++i) {
        Option call;
        call.symbol = QString("C%1").arg(90 + i * 5);
        call.strike = 90.0 + i * 5.0;
        call.side = Side::Call;
        call.bid = 3.0 - i * 0.5;
        call.ask = 3.5 - i * 0.5;
        call.dte = 30;
        call.expiry = 1000000;
        call.impliedVolatility = 0.25;
        call.delta = 0.5;
        chain.calls.push_back(call);

        Option put;
        put.symbol = QString("P%1").arg(90 + i * 5);
        put.strike = 90.0 + i * 5.0;
        put.side = Side::Put;
        put.bid = 1.0 + i * 0.5;
        put.ask = 1.5 + i * 0.5;
        put.dte = 30;
        put.expiry = 1000000;
        put.impliedVolatility = 0.25;
        put.delta = -0.3;
        chain.puts.push_back(put);
    }
    return chain;
}

TEST(TradeBuilderTest, SpreadsNotEmpty) {
    TradeBuilder tb(makeChain());
    auto spreads = tb.spreads();
    EXPECT_GT(spreads.size(), 0u);
}

TEST(TradeBuilderTest, SpreadsHaveOneByOneLegs) {
    TradeBuilder tb(makeChain());
    auto spreads = tb.spreads();
    for (const auto& t : spreads) {
        EXPECT_EQ(t->getBuys().size(), 1u);
        EXPECT_EQ(t->getSells().size(), 1u);
    }
}

TEST(TradeBuilderTest, BullPutSpreadsValid) {
    TradeBuilder tb(makeChain());
    auto bps = tb.bullPutSpreads();
    EXPECT_GT(bps.size(), 0u);
    for (const auto& t : bps) {
        EXPECT_EQ(t->getBuys().size(), 1u);
        EXPECT_EQ(t->getSells().size(), 1u);
        EXPECT_EQ(t->getBuys()[0].side, Side::Put);
        EXPECT_EQ(t->getSells()[0].side, Side::Put);
        EXPECT_LT(t->getBuys()[0].strike,
                   t->getSells()[0].strike);
    }
}

TEST(TradeBuilderTest, CondorsHaveFourLegs) {
    TradeBuilder tb(makeChain());
    auto condors = tb.condors();
    EXPECT_GT(condors.size(), 0u);
    for (const auto& t : condors) {
        EXPECT_EQ(t->getBuys().size(), 2u);
        EXPECT_EQ(t->getSells().size(), 2u);
    }
}

TEST(TradeBuilderTest, ThreeLegTradesHaveThreeLegs) {
    TradeBuilder tb(makeChain());
    auto trades = tb.threeLegTrades();
    EXPECT_GT(trades.size(), 0u);
    for (const auto& t : trades) {
        auto total = t->getBuys().size() + t->getSells().size();
        EXPECT_EQ(total, 3u);
    }
}

TEST(TradeBuilderTest, UnderlyingPrice) {
    auto chain = makeChain();
    TradeBuilder tb(chain);
    EXPECT_EQ(tb.underlyingPrice(), 100.0);
}

} // namespace
