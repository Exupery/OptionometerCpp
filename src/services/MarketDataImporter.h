#pragma once

#include "models/OptionChain.h"
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <vector>
#include <functional>

struct MarketDataResult {
    std::vector<OptionChain> chains;
    int rateLimitRemaining = -1;
    QString errorMessage;
};

class MarketDataImporter : public QObject {
    Q_OBJECT
public:
    explicit MarketDataImporter(QObject* parent = nullptr);

    void fetchOptionChains(
        const QString& ticker, const QString& apiToken,
        int minDte, int maxDte, int maxStrikes,
        bool fridaysOnly);

    static MarketDataResult parseResponse(
        const QByteArray& data, const QString& ticker,
        bool fridaysOnly);

    static void appendToLog(const QString& message);

signals:
    void finished(MarketDataResult result);

private:
    static qint64 dteToEpoch(int dte);
    static bool expiresOnFriday(qint64 epoch);

    QNetworkAccessManager* m_nam;
};
