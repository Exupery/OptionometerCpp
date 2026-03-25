#include <gtest/gtest.h>
#include "services/MarketDataImporter.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace {

QByteArray makeSampleResponse() {
    QJsonObject root;
    root["s"] = "ok";

    QJsonArray symbols, sides, strikes, expirations, dtes;
    QJsonArray bids, asks, ivs, deltas, prices;

    auto addOption = [&](const QString& sym, const QString& side,
                         double strike, int dte, double bid,
                         double ask, double iv, double delta,
                         qint64 expiry)
    {
        symbols.append(sym);
        sides.append(side);
        strikes.append(strike);
        dtes.append(dte);
        bids.append(bid);
        asks.append(ask);
        ivs.append(iv);
        deltas.append(delta);
        expirations.append(static_cast<double>(expiry));
        prices.append(100.0);
    };

    qint64 expiry1 = 1700000000;
    addOption("SPY231215C100", "call", 100, 30,
              2.0, 2.5, 0.25, 0.5, expiry1);
    addOption("SPY231215P100", "put", 100, 30,
              2.0, 2.5, 0.25, -0.5, expiry1);
    addOption("SPY231215C105", "call", 105, 30,
              1.0, 1.5, 0.25, 0.3, expiry1);
    addOption("SPY231215P95", "put", 95, 30,
              1.0, 1.5, 0.25, -0.3, expiry1);
    // Zero IV should be filtered
    addOption("SPY231215C110", "call", 110, 30,
              0.5, 1.0, 0.0, 0.1, expiry1);

    root["optionSymbol"] = symbols;
    root["side"] = sides;
    root["strike"] = strikes;
    root["expiration"] = expirations;
    root["dte"] = dtes;
    root["bid"] = bids;
    root["ask"] = asks;
    root["iv"] = ivs;
    root["delta"] = deltas;
    root["underlyingPrice"] = prices;

    return QJsonDocument(root).toJson();
}

TEST(MarketDataImporterTest, ParsesValidResponse) {
    auto data = makeSampleResponse();
    auto result = MarketDataImporter::parseResponse(
        data, "SPY", false);
    EXPECT_TRUE(result.errorMessage.isEmpty());
    EXPECT_EQ(result.chains.size(), 1u);
}

TEST(MarketDataImporterTest, FiltersZeroIV) {
    auto data = makeSampleResponse();
    auto result = MarketDataImporter::parseResponse(
        data, "SPY", false);
    auto& chain = result.chains[0];
    // 5 options total, 1 has IV=0 -> 4 remain
    auto total = chain.calls.size() + chain.puts.size();
    EXPECT_EQ(total, 4u);
}

TEST(MarketDataImporterTest, SetsUnderlyingPrice) {
    auto data = makeSampleResponse();
    auto result = MarketDataImporter::parseResponse(
        data, "SPY", false);
    EXPECT_NEAR(result.chains[0].underlyingPrice, 100.0, 0.01);
}

TEST(MarketDataImporterTest, SortsByStrike) {
    auto data = makeSampleResponse();
    auto result = MarketDataImporter::parseResponse(
        data, "SPY", false);
    auto& chain = result.chains[0];
    if (chain.calls.size() >= 2) {
        EXPECT_LE(chain.calls[0].strike,
                  chain.calls[1].strike);
    }
}

TEST(MarketDataImporterTest, HandlesEmptyResponse) {
    QJsonObject root;
    root["strike"] = QJsonArray();
    root["underlyingPrice"] = QJsonArray();
    auto data = QJsonDocument(root).toJson();
    auto result = MarketDataImporter::parseResponse(
        data, "SPY", false);
    EXPECT_FALSE(result.errorMessage.isEmpty());
}

TEST(MarketDataImporterTest, HandlesInvalidJson) {
    auto result = MarketDataImporter::parseResponse(
        "not json", "SPY", false);
    EXPECT_FALSE(result.errorMessage.isEmpty());
}

TEST(MarketDataImporterTest, SetsCorrectTicker) {
    auto data = makeSampleResponse();
    auto result = MarketDataImporter::parseResponse(
        data, "QQQ", false);
    EXPECT_EQ(result.chains[0].underlying, "QQQ");
}

} // namespace
