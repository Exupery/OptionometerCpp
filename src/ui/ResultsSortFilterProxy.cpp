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
    emit headerDataChanged(Qt::Horizontal, 0, columnCount() - 1);
}

QVariant ResultsSortFilterProxy::headerData(
    int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal
        && role == Qt::DisplayRole
        && m_lastSortColumn >= 0) {
        // Map proxy section to source column, accounting for
        // hidden columns
        int sourceCol = -1;
        if (rowCount() > 0) {
            sourceCol = mapToSource(index(0, section)).column();
        } else {
            // No rows: count visible source columns to find
            // which source column this section maps to
            int visible = 0;
            for (int i = 0;
                 i < sourceModel()->columnCount(); ++i) {
                if (!m_hiddenColumns.contains(i)) {
                    if (visible == section) {
                        sourceCol = i;
                        break;
                    }
                    ++visible;
                }
            }
        }
        if (sourceCol == m_lastSortColumn) {
            QString name = sourceModel()->headerData(
                sourceCol, orientation, role).toString();
            name += (m_currentOrder == Qt::AscendingOrder)
                ? QStringLiteral(" \u25B2")
                : QStringLiteral(" \u25BC");
            return name;
        }
    }
    return QSortFilterProxyModel::headerData(
        section, orientation, role);
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

    if (leftData.typeId() == QMetaType::Double
        && rightData.typeId() == QMetaType::Double) {
        return leftData.toDouble() < rightData.toDouble();
    }
    if (leftData.typeId() == QMetaType::Int
        && rightData.typeId() == QMetaType::Int) {
        return leftData.toInt() < rightData.toInt();
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
