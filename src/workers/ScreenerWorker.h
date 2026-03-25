#pragma once

#include "services/MarketDataImporter.h"
#include "services/ScreenerService.h"
#include <QObject>
#include <QThread>

class ScreenerWorker : public QObject {
    Q_OBJECT
public:
    explicit ScreenerWorker(QObject* parent = nullptr);
    ~ScreenerWorker() override;

public slots:
    void runScreen(ScreenerParams params);

signals:
    void resultsReady(ScreenerResult result, QString ticker,
                      int mode);
    void rateLimitUpdated(int remaining);
    void errorOccurred(QString message);
    void statusUpdate(QString message);

private:
    QThread m_thread;
};
