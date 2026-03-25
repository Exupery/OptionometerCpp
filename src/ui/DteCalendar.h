#pragma once

#include <QCalendarWidget>
#include <QDate>
#include <QSet>

class DteCalendar : public QCalendarWidget {
    Q_OBJECT
public:
    explicit DteCalendar(QWidget* parent = nullptr);

    void setDteRange(int minDays, int maxDays);
    void setClosedDates(const QSet<QDate>& dates);

protected:
    void paintCell(
        QPainter* painter,
        const QRect& rect,
        QDate date) const override;

private:
    QDate m_minDate;
    QDate m_maxDate;
    QSet<QDate> m_closedDates;
};
