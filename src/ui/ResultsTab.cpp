#include "ResultsTab.h"
#include <QHeaderView>
#include <QMenu>
#include <QVBoxLayout>

ResultsTab::ResultsTab(
    const QStringList& hiddenColumnNames, QWidget* parent)
    : QWidget(parent)
    , m_hiddenColumnNames(hiddenColumnNames)
{
    m_model = new ResultsTableModel(this);
    m_proxy = new ResultsSortFilterProxy(this);
    m_proxy->setSourceModel(m_model);
    m_delegate = new ColorDelegate(this);
    setupUi();
}

void ResultsTab::setupUi() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_table = new QTableView(this);
    m_table->setModel(m_proxy);
    m_table->setItemDelegate(m_delegate);
    m_table->setAlternatingRowColors(true);
    m_table->setSelectionBehavior(
        QAbstractItemView::SelectRows);
    m_table->setSortingEnabled(false);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->verticalHeader()->setVisible(false);

    connect(m_table->horizontalHeader(),
        &QHeaderView::sectionClicked, this,
        [this](int logicalIndex) {
            int sourceCol = m_proxy->mapToSource(
                m_proxy->index(0, logicalIndex)).column();
            m_proxy->sortByColumn(sourceCol);
        });

    m_table->horizontalHeader()->setContextMenuPolicy(
        Qt::CustomContextMenu);
    connect(m_table->horizontalHeader(),
        &QHeaderView::customContextMenuRequested,
        this, [this](const QPoint& pos) { setupColumnMenu(); });

    layout->addWidget(m_table);
}

void ResultsTab::setScoredTrades(
    const std::vector<ScoredTrade>& trades)
{
    m_model->setScoredTrades(trades);
    applyHiddenColumns();
    sortByDefaultColumn();
}

void ResultsTab::setScoredBullPuts(
    const std::vector<ScoredBullPut>& trades)
{
    m_model->setScoredBullPuts(trades);
    applyHiddenColumns();
    sortByDefaultColumn();
}

QStringList ResultsTab::hiddenColumnNames() const {
    return m_hiddenColumnNames;
}

void ResultsTab::setupColumnMenu() {
    QMenu menu(this);
    auto names = m_model->columnNames();
    for (int i = 0; i < names.size(); ++i) {
        auto* action = menu.addAction(names[i]);
        action->setCheckable(true);
        action->setChecked(
            !m_hiddenColumnNames.contains(names[i]));
        connect(action, &QAction::toggled, this,
            [this, name = names[i]](bool visible) {
                if (visible)
                    m_hiddenColumnNames.removeAll(name);
                else if (!m_hiddenColumnNames.contains(name))
                    m_hiddenColumnNames.append(name);
                applyHiddenColumns();
                emit hiddenColumnsChanged(m_hiddenColumnNames);
            });
    }
    menu.exec(QCursor::pos());
}

void ResultsTab::applyHiddenColumns() {
    auto names = m_model->columnNames();
    QSet<int> hidden;
    for (int i = 0; i < names.size(); ++i) {
        if (m_hiddenColumnNames.contains(names[i]))
            hidden.insert(i);
    }
    m_proxy->setHiddenColumns(hidden);
}

void ResultsTab::sortByDefaultColumn() {
    auto names = m_model->columnNames();
    int annualizedCol = names.indexOf("Annualized");
    if (annualizedCol >= 0) {
        m_proxy->sortByColumn(annualizedCol);
    }
}
