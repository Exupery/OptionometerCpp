#include "DteCalendar.h"
#include <QPainter>

DteCalendar::DteCalendar(QWidget* parent)
    : QCalendarWidget(parent)
{
    setSelectionMode(QCalendarWidget::NoSelection);
    setNavigationBarVisible(true);
    setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);
    setHorizontalHeaderFormat(QCalendarWidget::ShortDayNames);
    setGridVisible(false);
    setDateEditEnabled(false);

    // Prevent user interaction
    setFocusPolicy(Qt::NoFocus);
}

void DteCalendar::setDteRange(int minDays, int maxDays) {
    QDate today = QDate::currentDate();
    m_minDate = today.addDays(minDays);
    m_maxDate = today.addDays(maxDays);
    updateCells();
}

void DteCalendar::paintCell(
    QPainter* painter,
    const QRect& rect,
    QDate date) const
{
    painter->save();

    bool inRange = (date >= m_minDate && date <= m_maxDate);
    bool isCurrentMonth =
        (date.month() == monthShown()
         && date.year() == yearShown());

    if (!isCurrentMonth) {
        painter->setPen(QColor(80, 80, 80));
        painter->fillRect(rect, QColor(30, 30, 30));
    } else if (inRange) {
        painter->fillRect(rect, QColor(42, 130, 218));
        painter->setPen(Qt::white);
    } else {
        painter->fillRect(rect, QColor(240, 240, 240));
        painter->setPen(QColor(42, 130, 218));
    }

    painter->drawText(rect, Qt::AlignCenter,
        QString::number(date.day()));
    painter->restore();
}
