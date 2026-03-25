#include "ScreenerWorker.h"
#include <QEventLoop>
#include <QNetworkAccessManager>

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
        emit errorOccurred("No options matching criteria.");
        return;
    }

    emit statusUpdate(QString("Scoring %1 chain(s)...")
        .arg(fetchResult.chains.size()));

    auto result = ScreenerService::runScreen(
        fetchResult.chains, params);

    emit resultsReady(result, params.ticker,
        static_cast<int>(params.mode));
}
