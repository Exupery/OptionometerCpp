#include "MainWindow.h"
#include "ResultsTab.h"
#include "ChartWindow.h"
#include <QAbstractSpinBox>
#include <QApplication>
#include <QCloseEvent>
#include <QComboBox>
#include <QFile>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QSplitter>
#include <QVBoxLayout>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>
#include <QWheelEvent>

// Prevents mousewheel from changing spinbox/combobox values
class WheelGuard : public QObject {
public:
    using QObject::QObject;
protected:
    bool eventFilter(QObject* obj, QEvent* event) override {
        if (event->type() == QEvent::Wheel) {
            if (qobject_cast<QAbstractSpinBox*>(obj)
                || qobject_cast<QComboBox*>(obj)) {
                event->ignore();
                return true;
            }
        }
        return false;
    }
};

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_marketStatusNam(new QNetworkAccessManager(this))
{
    m_settings = m_settingsManager.load();
    loadApiTokenFromFile();
    setupUi();
    setupMenuBar();
    setupConnections();

    // Disable mousewheel mutation on spinboxes and comboboxes
    qApp->installEventFilter(new WheelGuard(this));
    m_screenerPanel->restoreFromSettings(m_settings);
    setWindowTitle("Optionometer");
    resize(1200, 700);

    // Initial market status fetch for current DTE range
    fetchMarketStatus(
        m_screenerPanel->minDays(),
        m_screenerPanel->maxDays());
}

MainWindow::~MainWindow() {
    delete m_worker;
}

void MainWindow::setupUi() {
    auto* splitter = new QSplitter(Qt::Horizontal, this);

    m_screenerPanel = new ScreenerPanel(this);
    m_screenerPanel->setMinimumWidth(280);
    m_screenerPanel->setMaximumWidth(350);
    splitter->addWidget(m_screenerPanel);

    m_stack = new QStackedWidget(this);

    m_welcomeLabel = new QLabel(
        "Configure parameters and click Screen to begin.",
        this);
    m_welcomeLabel->setAlignment(Qt::AlignCenter);
    m_stack->addWidget(m_welcomeLabel);

    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setTabsClosable(true);
    m_stack->addWidget(m_tabWidget);

    splitter->addWidget(m_stack);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    setCentralWidget(splitter);

    m_statusLabel = new QLabel("", this);
    statusBar()->addPermanentWidget(m_statusLabel);
}

void MainWindow::setupMenuBar() {
    auto* settingsAction = menuBar()->addAction("Settings");
    connect(settingsAction, &QAction::triggered,
        this, &MainWindow::openSettings);
}

void MainWindow::setupConnections() {
    m_worker = new ScreenerWorker();

    connect(m_tabWidget, &QTabWidget::tabCloseRequested,
        this, [this](int index) {
            auto* tab = qobject_cast<ResultsTab*>(
                m_tabWidget->widget(index));
            if (tab) {
                // Delete all chart windows for this tab
                auto it = m_chartWindows.find(tab);
                if (it != m_chartWindows.end()) {
                    for (auto* w : it.value())
                        w->deleteLater();
                    m_chartWindows.erase(it);
                }
            }
            m_tabWidget->removeTab(index);
            updateTabVisibility();
        });

    connect(m_tabWidget, &QTabWidget::currentChanged,
        this, &MainWindow::onTabChanged);

    connect(m_screenerPanel, &ScreenerPanel::screenRequested,
        this, &MainWindow::onScreenRequested);

    connect(m_screenerPanel, &ScreenerPanel::dteRangeExtended,
        this, &MainWindow::onDteRangeExtended);

    connect(m_screenerPanel, &ScreenerPanel::showDteHalfChanged,
        this, [this](bool checked) {
            m_settings.showDteHalf = checked;
            m_settingsManager.save(m_settings);
            for (auto& tabWindows : m_chartWindows)
                for (auto* w : tabWindows)
                    w->setShowDteHalf(checked);
        });

    connect(m_worker, &ScreenerWorker::resultsReady,
        this, &MainWindow::onResultsReady,
        Qt::QueuedConnection);

    connect(m_worker, &ScreenerWorker::errorOccurred,
        this, &MainWindow::onError,
        Qt::QueuedConnection);

    connect(m_worker, &ScreenerWorker::rateLimitUpdated,
        this, [this](int remaining) {
            m_screenerPanel->setRateLimitRemaining(remaining);
        }, Qt::QueuedConnection);

    connect(m_worker, &ScreenerWorker::statusUpdate,
        this, [this](const QString& msg) {
            m_statusLabel->setText(msg);
        }, Qt::QueuedConnection);
}

void MainWindow::openSettings() {
    SettingsDialog dialog(m_settings, this);
    if (dialog.exec() == QDialog::Accepted) {
        auto newSettings = dialog.getSettings();
        // Preserve screener and column state
        m_screenerPanel->saveToSettings(m_settings);
        newSettings.lastTicker = m_settings.lastTicker;
        newSettings.lastMode = m_settings.lastMode;
        newSettings.lastTradeType = m_settings.lastTradeType;
        newSettings.lastMinProbability =
            m_settings.lastMinProbability;
        newSettings.lastMinDays = m_settings.lastMinDays;
        newSettings.lastMaxDays = m_settings.lastMaxDays;
        newSettings.lastFridaysOnly = m_settings.lastFridaysOnly;
        newSettings.hiddenColumns = m_settings.hiddenColumns;
        m_settings = newSettings;
        m_settingsManager.save(m_settings);
    }
}

void MainWindow::onScreenRequested() {
    m_screenerPanel->saveToSettings(m_settings);
    m_settingsManager.save(m_settings);

    auto params = m_screenerPanel->currentParams(m_settings);
    m_screenerPanel->setScreening(true);
    m_statusLabel->setText("Starting screen...");

    QMetaObject::invokeMethod(m_worker, "runScreen",
        Qt::QueuedConnection, Q_ARG(ScreenerParams, params));
}

void MainWindow::onResultsReady(
    ScreenerResult result, QString ticker, int mode)
{
    m_screenerPanel->setScreening(false);
    m_statusLabel->setText("Screen complete.");

    QString modeStr;
    switch (static_cast<ScreenerMode>(mode)) {
    case ScreenerMode::StrategyOptimizer:
        modeStr = "Strategy"; break;
    case ScreenerMode::BullPutSpreadScreener:
        modeStr = "BPS Screen"; break;
    case ScreenerMode::BullPutSpreadOptimizer:
        modeStr = "BPS Optim"; break;
    }

    if (auto* trades = std::get_if<
            std::vector<std::vector<ScoredTrade>>>(&result)) {
        for (const auto& expiry : *trades) {
            if (expiry.empty()) continue;
            auto* tab = new ResultsTab(
                m_settings.hiddenColumns, this);
            tab->setScoredTrades(expiry);

            int dte = expiry.front().trade
                ? expiry.front().trade->getSells().front().dte : 0;
            QString tabName = QString("%1 %2 %3d")
                .arg(ticker, modeStr).arg(dte);
            int idx = m_tabWidget->addTab(tab, tabName);
            m_tabWidget->setTabToolTip(idx,
                QDate::currentDate().addDays(dte)
                    .toString(Qt::ISODate));
            m_tabWidget->setCurrentIndex(idx);

            connectResultsTab(tab, ticker);
        }
    } else if (auto* bps = std::get_if<
            std::vector<std::vector<ScoredBullPut>>>(&result)) {
        for (const auto& expiry : *bps) {
            if (expiry.empty()) continue;
            auto* tab = new ResultsTab(
                m_settings.hiddenColumns, this);
            tab->setScoredBullPuts(expiry);

            int dte = expiry.front().trade
                ? expiry.front().trade->getSells().front().dte : 0;
            QString tabName = QString("%1 %2 %3d")
                .arg(ticker, modeStr).arg(dte);
            int idx = m_tabWidget->addTab(tab, tabName);
            m_tabWidget->setTabToolTip(idx,
                QDate::currentDate().addDays(dte)
                    .toString(Qt::ISODate));
            m_tabWidget->setCurrentIndex(idx);

            connectResultsTab(tab, ticker);
        }
    }

    updateTabVisibility();
}

void MainWindow::onError(const QString& message) {
    m_screenerPanel->setScreening(false);
    m_statusLabel->setText("Error.");
    QMessageBox::warning(this, "Screening Error", message);
}

void MainWindow::onHiddenColumnsChanged(
    const QStringList& hidden)
{
    m_settings.hiddenColumns = hidden;
    m_settingsManager.save(m_settings);
}

void MainWindow::closeEvent(QCloseEvent* event) {
    m_screenerPanel->saveToSettings(m_settings);
    m_settingsManager.save(m_settings);
    event->accept();
}

void MainWindow::loadApiTokenFromFile() {
    if (!m_settings.apiToken.isEmpty()) return;

    QFile tokenFile(".marketdataapitoken");
    if (tokenFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString token = tokenFile.readLine().trimmed();
        if (!token.isEmpty()) {
            m_settings.apiToken = token;
        }
    }
}

void MainWindow::onDteRangeExtended(int minDays, int maxDays) {
    // Only re-fetch if the range actually extends beyond
    // what we've already fetched
    if (minDays >= m_fetchedMinDay && maxDays <= m_fetchedMaxDay)
        return;
    fetchMarketStatus(minDays, maxDays);
}

void MainWindow::updateTabVisibility() {
    if (m_tabWidget->count() > 0)
        m_stack->setCurrentWidget(m_tabWidget);
    else
        m_stack->setCurrentWidget(m_welcomeLabel);
}

void MainWindow::connectResultsTab(
    ResultsTab* tab, const QString& ticker)
{
    tab->setTicker(ticker);

    connect(tab, &ResultsTab::hiddenColumnsChanged,
        this, &MainWindow::onHiddenColumnsChanged);

    connect(tab, &ResultsTab::chartRequested, this,
        [this, tab](int sourceRow, PlChartWidget::Data data,
                    QString title) {
            onChartRequested(tab, sourceRow, data, title);
        });
}

void MainWindow::onChartRequested(
    ResultsTab* tab, int sourceRow,
    const PlChartWidget::Data& data, const QString& title)
{
    // Check if window already open for this row
    auto& tabWindows = m_chartWindows[tab];
    if (tabWindows.contains(sourceRow)) {
        tabWindows[sourceRow]->raise();
        return;
    }

    // Create chart window as child of the stack widget
    QSize savedSize(m_settings.chartWidth, m_settings.chartHeight);
    auto* chartWin = new ChartWindow(title, data, m_stack,
                                     savedSize);
    chartWin->setShowDteHalf(m_settings.showDteHalf);

    tabWindows[sourceRow] = chartWin;

    connect(chartWin, &ChartWindow::closed, this,
        [this, tab, sourceRow]() {
            m_chartWindows[tab].remove(sourceRow);
        });

    connect(chartWin, &ChartWindow::sizeChanged, this,
        [this](const QSize& sz) {
            m_settings.chartWidth = sz.width();
            m_settings.chartHeight = sz.height();
            m_settingsManager.save(m_settings);
        });
}

void MainWindow::onTabChanged(int index) {
    // Hide all chart windows, then show ones for current tab
    for (auto it = m_chartWindows.begin();
         it != m_chartWindows.end(); ++it)
    {
        bool isCurrent =
            (m_tabWidget->widget(index) == it.key());
        for (auto* w : it.value()) {
            w->setVisible(isCurrent);
            if (isCurrent) w->raise();
        }
    }
}

void MainWindow::fetchMarketStatus(int minDays, int maxDays) {
    if (m_settings.apiToken.isEmpty()) return;

    // Determine the actual range to fetch — union of old and new
    int fetchMin = (m_fetchedMinDay > 0)
        ? std::min(minDays, m_fetchedMinDay) : minDays;
    int fetchMax = std::max(maxDays, m_fetchedMaxDay);

    QDate today = QDate::currentDate();
    QDate fromDate = today.addDays(fetchMin);
    QDate toDate = today.addDays(fetchMax);

    QUrl url("https://api.marketdata.app/v1/markets/status/");
    QUrlQuery query;
    query.addQueryItem("from", fromDate.toString(Qt::ISODate));
    query.addQueryItem("to", toDate.toString(Qt::ISODate));
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setRawHeader("Authorization",
        ("Bearer " + m_settings.apiToken).toUtf8());

    auto* reply = m_marketStatusNam->get(request);
    connect(reply, &QNetworkReply::finished, this,
        [this, reply, fetchMin, fetchMax]() {
            reply->deleteLater();

            if (reply->error() != QNetworkReply::NoError)
                return;

            auto data = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            if (!doc.isObject()) return;

            QJsonObject root = doc.object();
            if (root["s"].toString() != "ok") return;

            auto dates = root["date"].toArray();
            auto statuses = root["status"].toArray();

            for (int i = 0; i < dates.size(); ++i) {
                if (statuses[i].toString() == "closed") {
                    qint64 epoch = static_cast<qint64>(
                        dates[i].toDouble());
                    QDate d = QDateTime::fromSecsSinceEpoch(
                        epoch, Qt::UTC).date();
                    // Only add weekday closures (holidays)
                    if (d.dayOfWeek() >= 1 && d.dayOfWeek() <= 5) {
                        m_closedDates.insert(d);
                    }
                }
            }

            m_fetchedMinDay = fetchMin;
            m_fetchedMaxDay = fetchMax;
            m_screenerPanel->setClosedDates(m_closedDates);
        });
}
