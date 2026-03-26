#pragma once

#include "ResultsTableModel.h"
#include "ResultsSortFilterProxy.h"
#include "ColorDelegate.h"
#include "PlChartWidget.h"
#include "models/ScoredTrade.h"
#include "models/ScoredBullPut.h"
#include <QWidget>
#include <QTableView>
#include <QSet>

class ResultsTab : public QWidget {
    Q_OBJECT
public:
    explicit ResultsTab(
        const QStringList& hiddenColumnNames,
        QWidget* parent = nullptr);

    void setScoredTrades(
        const std::vector<ScoredTrade>& trades);
    void setScoredBullPuts(
        const std::vector<ScoredBullPut>& trades);
    void setTicker(const QString& ticker);

    QStringList hiddenColumnNames() const;

signals:
    void hiddenColumnsChanged(QStringList hiddenNames);
    void chartRequested(int sourceRow,
                        PlChartWidget::Data data,
                        QString title);

private:
    void setupUi();
    void setupColumnMenu();
    void applyHiddenColumns();
    void sortByDefaultColumn();
    void onDoubleClicked(const QModelIndex& proxyIndex);

    QTableView* m_table;
    ResultsTableModel* m_model;
    ResultsSortFilterProxy* m_proxy;
    ColorDelegate* m_delegate;
    QStringList m_hiddenColumnNames;
    QString m_ticker;
};
