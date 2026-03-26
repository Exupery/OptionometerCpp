#pragma once

#include "models/ScoredTrade.h"
#include "models/ScoredBullPut.h"
#include "PlChartWidget.h"
#include <QAbstractTableModel>
#include <QStringList>
#include <variant>
#include <vector>

class ResultsTableModel : public QAbstractTableModel {
    Q_OBJECT
public:
    explicit ResultsTableModel(QObject* parent = nullptr);

    void setScoredTrades(
        const std::vector<ScoredTrade>& trades);
    void setScoredBullPuts(
        const std::vector<ScoredBullPut>& trades);

    int rowCount(
        const QModelIndex& parent = {}) const override;
    int columnCount(
        const QModelIndex& parent = {}) const override;
    QVariant data(
        const QModelIndex& index,
        int role = Qt::DisplayRole) const override;
    QVariant headerData(
        int section, Qt::Orientation orientation,
        int role = Qt::DisplayRole) const override;

    bool isNumericColumn(int column) const;
    QStringList columnNames() const;

    PlChartWidget::Data chartDataForRow(int row) const;
    QString tradeStringForRow(int row) const;

private:
    QVariant tradeData(int row, int col, int role) const;
    QVariant bullPutData(int row, int col, int role) const;
    QColor colorForValue(double value) const;

    std::vector<ScoredTrade> m_trades;
    std::vector<ScoredBullPut> m_bullPuts;
    bool m_isBullPut = false;
    QStringList m_tradeHeaders;
    QStringList m_bullPutHeaders;
};
