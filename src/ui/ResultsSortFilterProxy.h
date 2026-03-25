#pragma once

#include <QSortFilterProxyModel>
#include <QSet>

class ResultsTableModel;

class ResultsSortFilterProxy : public QSortFilterProxyModel {
    Q_OBJECT
public:
    explicit ResultsSortFilterProxy(QObject* parent = nullptr);

    void sortByColumn(int column);
    void setHiddenColumns(const QSet<int>& columns);
    QSet<int> hiddenColumns() const { return m_hiddenColumns; }

protected:
    bool lessThan(
        const QModelIndex& left,
        const QModelIndex& right) const override;
    bool filterAcceptsColumn(
        int column,
        const QModelIndex& parent) const override;

private:
    bool isNumericColumn(int column) const;
    int m_lastSortColumn = -1;
    Qt::SortOrder m_currentOrder = Qt::DescendingOrder;
    QSet<int> m_hiddenColumns;
};
