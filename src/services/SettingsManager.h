#pragma once

#include <QString>
#include <QStringList>
#include <nlohmann/json.hpp>

struct AppSettings {
    // API
    QString apiToken;

    // Screener (persisted for session restore)
    QString lastTicker = "SPY";
    int lastMode = 0;
    int lastTradeType = 0;
    int lastMinProbability = 25;
    int lastMinDays = 25;
    int lastMaxDays = 45;
    bool lastFridaysOnly = false;

    // Settings panel
    int maxStrikes = 20;
    double commission = 0.5;
    double minAnnualReturn = 1.0;
    double minProfitAmount = 0.5;
    int pricePointWeight = 1;
    int profitPointWeight = 1;
    int probabilityWeight = 1;
    int profitLossWeight = 1;
    int annualReturnWeight = 1;
    int deltaWeight = 1;
    int hundredTradeWeight = 1;
    int maxMargin = 10000;
    int minBullPutStrikeBelow = 2;
    int targetSellStrike = 0;

    // Column visibility
    QStringList hiddenColumns;

    // Chart window geometry
    int chartWidth = 0;
    int chartHeight = 0;
};

class SettingsManager {
public:
    SettingsManager();

    AppSettings load() const;
    void save(const AppSettings& settings) const;

    static QString settingsFilePath();

private:
    static nlohmann::json toJson(const AppSettings& s);
    static AppSettings fromJson(const nlohmann::json& j);
    QString m_filePath;
};
