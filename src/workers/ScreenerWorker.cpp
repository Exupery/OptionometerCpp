#include "ScreenerWorker.h"
#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QStandardPaths>
#include <QDir>

static void writeDebugLog(const QString& reason,
                          const ScreenerParams& params,
                          int chainCount = 0,
                          int totalTrades = 0)
{
    QString appData = QStandardPaths::writableLocation(
        QStandardPaths::AppDataLocation);
    QDir().mkpath(appData);
    QString logPath = appData + "/screener_debug.log";

    QFile file(logPath);
    if (!file.open(QIODevice::Append | QIODevice::Text))
        return;

    QTextStream out(&file);
    out << "=== " << QDateTime::currentDateTime().toString(Qt::ISODate)
        << " ===\n";
    out << "Reason: " << reason << "\n";
    out << "Chains fetched: " << chainCount << "\n";
    out << "Total trades scored: " << totalTrades << "\n";
    out << "\n--- Screener Params ---\n";
    out << "Ticker: " << params.ticker << "\n";
    out << "Mode: " << static_cast<int>(params.mode) << "\n";
    out << "Trade Type: " << static_cast<int>(params.tradeType) << "\n";
    out << "Min Probability: " << params.minProbability << "\n";
    out << "Min Days: " << params.minDays << "\n";
    out << "Max Days: " << params.maxDays << "\n";
    out << "Fridays Only: " << (params.fridaysOnly ? "true" : "false") << "\n";
    out << "\n--- Settings ---\n";
    out << "Max Strikes: " << params.maxStrikes << "\n";
    out << "Commission: " << params.commission << "\n";
    out << "Min Annual Return: " << params.minAnnualReturn << "\n";
    out << "Min Profit Amount: " << params.minProfitAmount << "\n";
    out << "Price Point Weight: " << params.pricePointWeight << "\n";
    out << "Profit Point Weight: " << params.profitPointWeight << "\n";
    out << "Probability Weight: " << params.probabilityWeight << "\n";
    out << "Profit Loss Weight: " << params.profitLossWeight << "\n";
    out << "Annual Return Weight: " << params.annualReturnWeight << "\n";
    out << "Delta Weight: " << params.deltaWeight << "\n";
    out << "Hundred Trade Weight: " << params.hundredTradeWeight << "\n";
    out << "Max Margin: " << params.maxMargin << "\n";
    out << "Min Bull Put Strike Below: " << params.minBullPutStrikeBelow << "\n";
    out << "Target Sell Strike: " << params.targetSellStrike << "\n";
    out << "API Token: " << (params.apiToken.isEmpty() ? "(empty)" : "(set)") << "\n";
    out << "\n";
}

ScreenerWorker::ScreenerWorker(QObject* parent)
    : QObject(nullptr)
{
    Q_UNUSED(parent);
    this->moveToThread(&m_thread);
    m_thread.start();
}

ScreenerWorker::~ScreenerWorker() {
    m_thread.quit();
    m_thread.wait();
}

void ScreenerWorker::runScreen(ScreenerParams params) {
    emit statusUpdate("Fetching option chains...");

    if (params.apiToken.isEmpty()) {
        emit errorOccurred(
            "API token not set. Please configure it in Settings.");
        return;
    }

    if (params.minDays >= params.maxDays) {
        emit errorOccurred(
            "Min days must be less than max days.");
        return;
    }

    auto* importer = new MarketDataImporter();
    QEventLoop loop;
    MarketDataResult fetchResult;

    connect(importer, &MarketDataImporter::finished,
        [&](MarketDataResult result) {
            fetchResult = std::move(result);
            loop.quit();
        });

    importer->fetchOptionChains(
        params.ticker, params.apiToken,
        params.minDays, params.maxDays,
        params.maxStrikes, params.fridaysOnly);

    loop.exec();
    importer->deleteLater();

    if (fetchResult.rateLimitRemaining >= 0) {
        emit rateLimitUpdated(fetchResult.rateLimitRemaining);
    }

    if (!fetchResult.errorMessage.isEmpty()) {
        emit errorOccurred(fetchResult.errorMessage);
        return;
    }

    if (fetchResult.chains.empty()) {
        writeDebugLog("No options found matching criteria",
                      params, 0, 0);
        emit errorOccurred("No options matching criteria. "
            "Check screener_debug.log for details.");
        return;
    }

    emit statusUpdate(QString("Scoring %1 chain(s)...")
        .arg(fetchResult.chains.size()));

    auto result = ScreenerService::runScreen(
        fetchResult.chains, params);

    // Check if all result vectors are empty (no trades survived scoring)
    int totalTrades = 0;
    if (auto* trades = std::get_if<
            std::vector<std::vector<ScoredTrade>>>(&result)) {
        for (const auto& v : *trades) totalTrades += v.size();
    } else if (auto* bps = std::get_if<
            std::vector<std::vector<ScoredBullPut>>>(&result)) {
        for (const auto& v : *bps) totalTrades += v.size();
    }

    if (totalTrades == 0) {
        writeDebugLog("No trades found after scoring/filtering",
                      params,
                      static_cast<int>(fetchResult.chains.size()),
                      0);
        emit errorOccurred("No trades matching criteria. "
            "Check screener_debug.log for details.");
        return;
    }

    emit resultsReady(result, params.ticker,
        static_cast<int>(params.mode));
}
