#include "ResultsTableModel.h"
#include <QColor>

ResultsTableModel::ResultsTableModel(QObject* parent)
    : QAbstractTableModel(parent)
{
    m_tradeHeaders = {
        "Score", "Probability", "Annualized", "100 Trades",
        "Max Profit", "Max Loss", "Max P/L Ratio",
        "-1SD", "+1SD", "Delta", "Margin", "Trade"
    };
    m_bullPutHeaders = {
        "Score", "Probability", "Annualized",
        "Max Profit", "Max Loss", "Contracts", "Trade"
    };
}

void ResultsTableModel::setScoredTrades(
    const std::vector<ScoredTrade>& trades)
{
    beginResetModel();
    m_trades = trades;
    m_isBullPut = false;
    endResetModel();
}

void ResultsTableModel::setScoredBullPuts(
    const std::vector<ScoredBullPut>& trades)
{
    beginResetModel();
    m_bullPuts = trades;
    m_isBullPut = true;
    endResetModel();
}

int ResultsTableModel::rowCount(const QModelIndex&) const {
    return m_isBullPut
        ? static_cast<int>(m_bullPuts.size())
        : static_cast<int>(m_trades.size());
}

int ResultsTableModel::columnCount(const QModelIndex&) const {
    return m_isBullPut
        ? m_bullPutHeaders.size()
        : m_tradeHeaders.size();
}

QVariant ResultsTableModel::data(
    const QModelIndex& index, int role) const
{
    if (!index.isValid()) return {};
    return m_isBullPut
        ? bullPutData(index.row(), index.column(), role)
        : tradeData(index.row(), index.column(), role);
}

QVariant ResultsTableModel::headerData(
    int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal
        || role != Qt::DisplayRole)
        return {};
    const auto& headers = m_isBullPut
        ? m_bullPutHeaders : m_tradeHeaders;
    if (section < 0 || section >= headers.size()) return {};
    return headers[section];
}

bool ResultsTableModel::isNumericColumn(int column) const {
    const auto& headers = m_isBullPut
        ? m_bullPutHeaders : m_tradeHeaders;
    if (column < 0 || column >= headers.size()) return false;
    return headers[column] != "Trade";
}

QStringList ResultsTableModel::columnNames() const {
    return m_isBullPut ? m_bullPutHeaders : m_tradeHeaders;
}

QVariant ResultsTableModel::tradeData(
    int row, int col, int role) const
{
    if (row < 0 || row >= static_cast<int>(m_trades.size()))
        return {};
    const auto& t = m_trades[row];

    if (role == Qt::ForegroundRole && col < 11) {
        double val = 0.0;
        switch (col) {
        case 2: val = t.annualizedReturn; break;
        case 3: val = t.hundredTrades; break;
        case 4: val = t.maxProfitLoss.maxProfit; break;
        case 5: val = t.maxProfitLoss.maxLoss; break;
        default: return {};
        }
        return colorForValue(val);
    }

    if (role == Qt::UserRole) {
        switch (col) {
        case 0: return t.score;
        case 1: return t.successProbability;
        case 2: return t.annualizedReturn;
        case 3: return t.hundredTrades;
        case 4: return t.maxProfitLoss.maxProfit;
        case 5: return t.maxProfitLoss.maxLoss;
        case 6: return t.maxProfitLoss.maxProfitToMaxLossRatio;
        case 7: return t.sdPrices.lowerSd;
        case 8: return t.sdPrices.upperSd;
        case 9: return t.tradeDelta;
        case 10: return t.trade ? t.trade->requiredMargin() : 0.0;
        case 11: return t.trade ? t.trade->toString() : "";
        default: return {};
        }
    }

    if (role != Qt::DisplayRole) return {};
    switch (col) {
    case 0: return t.score;
    case 1: return QString::number(t.successProbability, 'f', 2);
    case 2: return QString::number(t.annualizedReturn, 'f', 2);
    case 3: return t.hundredTrades;
    case 4: return QString::number(
        t.maxProfitLoss.maxProfit, 'f', 2);
    case 5: return QString::number(
        t.maxProfitLoss.maxLoss, 'f', 2);
    case 6: return QString::number(
        t.maxProfitLoss.maxProfitToMaxLossRatio, 'f', 2);
    case 7: return QString::number(t.sdPrices.lowerSd, 'f', 2);
    case 8: return QString::number(t.sdPrices.upperSd, 'f', 2);
    case 9: return QString::number(t.tradeDelta, 'f', 4);
    case 10: return t.trade
        ? QString::number(t.trade->requiredMargin(), 'f', 2) : "";
    case 11: return t.trade ? t.trade->toString() : "";
    default: return {};
    }
}

QVariant ResultsTableModel::bullPutData(
    int row, int col, int role) const
{
    if (row < 0 || row >= static_cast<int>(m_bullPuts.size()))
        return {};
    const auto& t = m_bullPuts[row];

    if (role == Qt::ForegroundRole) {
        double val = 0.0;
        switch (col) {
        case 2: val = t.annualizedReturn; break;
        case 3: val = t.maxProfitLoss.maxProfit; break;
        case 4: val = t.maxProfitLoss.maxLoss; break;
        default: return {};
        }
        return colorForValue(val);
    }

    if (role == Qt::UserRole) {
        switch (col) {
        case 0: return t.score;
        case 1: return t.successProbability;
        case 2: return t.annualizedReturn;
        case 3: return t.maxProfitLoss.maxProfit;
        case 4: return t.maxProfitLoss.maxLoss;
        case 5: return t.numContracts;
        case 6: return t.trade ? t.trade->toString() : "";
        default: return {};
        }
    }

    if (role != Qt::DisplayRole) return {};
    switch (col) {
    case 0: return t.score;
    case 1: return QString::number(t.successProbability, 'f', 2);
    case 2: return QString::number(t.annualizedReturn, 'f', 2);
    case 3: return QString::number(
        t.maxProfitLoss.maxProfit, 'f', 2);
    case 4: return QString::number(
        t.maxProfitLoss.maxLoss, 'f', 2);
    case 5: return t.numContracts;
    case 6: return t.trade ? t.trade->toString() : "";
    default: return {};
    }
}

QColor ResultsTableModel::colorForValue(double value) const {
    if (value > 0.01) return QColor(76, 175, 80);
    if (value < -0.01) return QColor(244, 67, 54);
    return QColor(255, 235, 59);
}
