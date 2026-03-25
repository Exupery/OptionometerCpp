#pragma once

#include <QCalendarWidget>
#include <QDate>

class DteCalendar : public QCalendarWidget {
    Q_OBJECT
public:
    explicit DteCalendar(QWidget* parent = nullptr);

    void setDteRange(int minDays, int maxDays);

protected:
    void paintCell(
        QPainter* painter,
        const QRect& rect,
        QDate date) const override;

private:
    QDate m_minDate;
    QDate m_maxDate;
};
