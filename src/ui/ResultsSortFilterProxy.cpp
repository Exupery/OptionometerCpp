#include "ResultsSortFilterProxy.h"
#include "ResultsTableModel.h"

ResultsSortFilterProxy::ResultsSortFilterProxy(QObject* parent)
    : QSortFilterProxyModel(parent)
{}

void ResultsSortFilterProxy::sortByColumn(int column) {
    if (column == m_lastSortColumn) {
        m_currentOrder = (m_currentOrder == Qt::AscendingOrder)
            ? Qt::DescendingOrder : Qt::AscendingOrder;
    } else {
        m_currentOrder = isNumericColumn(column)
            ? Qt::DescendingOrder : Qt::AscendingOrder;
    }
    m_lastSortColumn = column;
    sort(column, m_currentOrder);
}

void ResultsSortFilterProxy::setHiddenColumns(
    const QSet<int>& columns)
{
    m_hiddenColumns = columns;
    invalidateFilter();
}

bool ResultsSortFilterProxy::lessThan(
    const QModelIndex& left,
    const QModelIndex& right) const
{
    QVariant leftData = sourceModel()->data(left, Qt::UserRole);
    QVariant rightData = sourceModel()->data(right, Qt::UserRole);

    if (leftData.canConvert<double>()
        && rightData.canConvert<double>()) {
        return leftData.toDouble() < rightData.toDouble();
    }
    return leftData.toString() < rightData.toString();
}

bool ResultsSortFilterProxy::filterAcceptsColumn(
    int column, const QModelIndex&) const
{
    return !m_hiddenColumns.contains(column);
}

bool ResultsSortFilterProxy::isNumericColumn(int column) const {
    auto* model = qobject_cast<ResultsTableModel*>(sourceModel());
    return model ? model->isNumericColumn(column) : true;
}
