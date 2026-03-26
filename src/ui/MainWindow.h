#pragma once

#include "ScreenerPanel.h"
#include "SettingsDialog.h"
#include "PlChartWidget.h"
#include "services/SettingsManager.h"
#include "services/ScreenerService.h"
#include "workers/ScreenerWorker.h"
#include <QMainWindow>
#include <QTabWidget>
#include <QStackedWidget>
#include <QLabel>
#include <QNetworkAccessManager>
#include <QSet>
#include <QDate>
#include <QMap>

class ChartWindow;
class ResultsTab;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void setupUi();
    void setupMenuBar();
    void setupConnections();
    void openSettings();
    void onScreenRequested();
    void onResultsReady(
        ScreenerResult result, QString ticker, int mode);
    void onError(const QString& message);
    void onHiddenColumnsChanged(const QStringList& hidden);
    void loadApiTokenFromFile();
    void fetchMarketStatus(int minDays, int maxDays);
    void onDteRangeExtended(int minDays, int maxDays);

    void updateTabVisibility();
    void onChartRequested(ResultsTab* tab, int sourceRow,
                          const PlChartWidget::Data& data,
                          const QString& title);
    void onTabChanged(int index);
    void connectResultsTab(ResultsTab* tab, const QString& ticker);

    SettingsManager m_settingsManager;
    AppSettings m_settings;
    QStackedWidget* m_stack;
    QLabel* m_welcomeLabel;
    QTabWidget* m_tabWidget;
    ScreenerPanel* m_screenerPanel;
    QLabel* m_statusLabel;
    ScreenerWorker* m_worker;
    QNetworkAccessManager* m_marketStatusNam;
    QSet<QDate> m_closedDates;
    int m_fetchedMinDay = 0;
    int m_fetchedMaxDay = 0;

    // Chart windows: tab -> (sourceRow -> ChartWindow*)
    QMap<ResultsTab*, QMap<int, ChartWindow*>> m_chartWindows;
};
