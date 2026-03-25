#include "SettingsManager.h"
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <fstream>

SettingsManager::SettingsManager()
    : m_filePath(settingsFilePath())
{}

QString SettingsManager::settingsFilePath() {
    QString appData = QStandardPaths::writableLocation(
        QStandardPaths::AppDataLocation);
    QDir dir(appData);
    if (!dir.exists()) dir.mkpath(".");
    return dir.filePath("settings.json");
}

AppSettings SettingsManager::load() const {
    QFile file(m_filePath);
    if (!file.exists()) return {};

    std::ifstream ifs(m_filePath.toStdString());
    if (!ifs.is_open()) return {};

    try {
        nlohmann::json j = nlohmann::json::parse(ifs);
        return fromJson(j);
    } catch (...) {
        return {};
    }
}

void SettingsManager::save(const AppSettings& settings) const {
    QDir dir(QFileInfo(m_filePath).path());
    if (!dir.exists()) dir.mkpath(".");

    std::ofstream ofs(m_filePath.toStdString());
    if (!ofs.is_open()) return;

    ofs << toJson(settings).dump(2);
}

nlohmann::json SettingsManager::toJson(const AppSettings& s) {
    nlohmann::json j;
    j["apiToken"] = s.apiToken.toStdString();
    j["lastTicker"] = s.lastTicker.toStdString();
    j["lastMode"] = s.lastMode;
    j["lastTradeType"] = s.lastTradeType;
    j["lastMinProbability"] = s.lastMinProbability;
    j["lastMinDays"] = s.lastMinDays;
    j["lastMaxDays"] = s.lastMaxDays;
    j["lastFridaysOnly"] = s.lastFridaysOnly;
    j["maxStrikes"] = s.maxStrikes;
    j["commission"] = s.commission;
    j["minAnnualReturn"] = s.minAnnualReturn;
    j["minProfitAmount"] = s.minProfitAmount;
    j["pricePointWeight"] = s.pricePointWeight;
    j["profitPointWeight"] = s.profitPointWeight;
    j["probabilityWeight"] = s.probabilityWeight;
    j["profitLossWeight"] = s.profitLossWeight;
    j["annualReturnWeight"] = s.annualReturnWeight;
    j["deltaWeight"] = s.deltaWeight;
    j["hundredTradeWeight"] = s.hundredTradeWeight;
    j["maxMargin"] = s.maxMargin;
    j["minBullPutStrikeBelow"] = s.minBullPutStrikeBelow;
    j["targetSellStrike"] = s.targetSellStrike;

    nlohmann::json cols = nlohmann::json::array();
    for (const auto& col : s.hiddenColumns) {
        cols.push_back(col.toStdString());
    }
    j["hiddenColumns"] = cols;
    return j;
}

AppSettings SettingsManager::fromJson(const nlohmann::json& j) {
    AppSettings s;
    auto str = [&](const char* key, QString def) {
        return j.contains(key)
            ? QString::fromStdString(j[key].get<std::string>())
            : def;
    };
    auto intVal = [&](const char* key, int def) {
        return j.value(key, def);
    };
    auto dblVal = [&](const char* key, double def) {
        return j.value(key, def);
    };
    auto boolVal = [&](const char* key, bool def) {
        return j.value(key, def);
    };

    s.apiToken = str("apiToken", "");
    s.lastTicker = str("lastTicker", "SPY");
    s.lastMode = intVal("lastMode", 0);
    s.lastTradeType = intVal("lastTradeType", 0);
    s.lastMinProbability = intVal("lastMinProbability", 25);
    s.lastMinDays = intVal("lastMinDays", 25);
    s.lastMaxDays = intVal("lastMaxDays", 45);
    s.lastFridaysOnly = boolVal("lastFridaysOnly", false);
    s.maxStrikes = intVal("maxStrikes", 20);
    s.commission = dblVal("commission", 0.5);
    s.minAnnualReturn = dblVal("minAnnualReturn", 1.0);
    s.minProfitAmount = dblVal("minProfitAmount", 0.5);
    s.pricePointWeight = intVal("pricePointWeight", 1);
    s.profitPointWeight = intVal("profitPointWeight", 1);
    s.probabilityWeight = intVal("probabilityWeight", 1);
    s.profitLossWeight = intVal("profitLossWeight", 1);
    s.annualReturnWeight = intVal("annualReturnWeight", 1);
    s.deltaWeight = intVal("deltaWeight", 1);
    s.hundredTradeWeight = intVal("hundredTradeWeight", 1);
    s.maxMargin = intVal("maxMargin", 10000);
    s.minBullPutStrikeBelow = intVal("minBullPutStrikeBelow", 2);
    s.targetSellStrike = intVal("targetSellStrike", 0);

    if (j.contains("hiddenColumns") && j["hiddenColumns"].is_array()) {
        for (const auto& col : j["hiddenColumns"]) {
            s.hiddenColumns.append(
                QString::fromStdString(col.get<std::string>()));
        }
    }
    return s;
}
