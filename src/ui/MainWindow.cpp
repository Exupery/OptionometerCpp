#include "MainWindow.h"
#include "ResultsTab.h"
#include <QApplication>
#include <QCloseEvent>
#include <QFile>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QSplitter>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    m_settings = m_settingsManager.load();
    loadApiTokenFromFile();
    setupUi();
    setupMenuBar();
    setupConnections();
    m_screenerPanel->restoreFromSettings(m_settings);
    setWindowTitle("Optionometer");
    resize(1200, 700);
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

    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setTabsClosable(true);

    auto* welcomeWidget = new QWidget(this);
    auto* wLayout = new QVBoxLayout(welcomeWidget);
    auto* welcomeLabel = new QLabel(
        "Configure parameters and click Screen to begin.",
        welcomeWidget);
    welcomeLabel->setAlignment(Qt::AlignCenter);
    wLayout->addWidget(welcomeLabel);
    m_tabWidget->addTab(welcomeWidget, "Screener");

    splitter->addWidget(m_tabWidget);
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
            if (index > 0) {
                m_tabWidget->removeTab(index);
            }
        });

    connect(m_screenerPanel, &ScreenerPanel::screenRequested,
        this, &MainWindow::onScreenRequested);

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
            m_tabWidget->setCurrentIndex(idx);

            connect(tab, &ResultsTab::hiddenColumnsChanged,
                this, &MainWindow::onHiddenColumnsChanged);
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
            m_tabWidget->setCurrentIndex(idx);

            connect(tab, &ResultsTab::hiddenColumnsChanged,
                this, &MainWindow::onHiddenColumnsChanged);
        }
    }
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
