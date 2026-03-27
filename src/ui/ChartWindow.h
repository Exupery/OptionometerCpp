#pragma once

#include "PlChartWidget.h"
#include <QWidget>

class ChartWindow : public QWidget {
    Q_OBJECT
public:
    ChartWindow(const QString& title,
                const PlChartWidget::Data& data,
                QWidget* parent,
                const QSize& savedSize = QSize());

    static QSize lastUsedSize();
    void setShowDteHalf(bool show);

signals:
    void closed();
    void sizeChanged(QSize newSize);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    enum HitZone { None, TitleBar, BottomLeft, BottomRight };
    HitZone hitTest(const QPoint& pos) const;
    void updateCursorShape(const QPoint& pos);

    PlChartWidget* m_chart;
    QString m_tickerPrice;
    QString m_tradeText;
    QRect m_closeButtonRect;

    HitZone m_activeZone = None;
    QPoint m_dragStartGlobal;
    QPoint m_windowStartPos;
    QSize m_windowStartSize;

    static QSize s_lastSize;
    static constexpr int kTitleBarHeight = 28;
    static constexpr int kResizeGrabSize = 30;
    static constexpr int kBorderSize = 6;
    static constexpr int kMinWidth = 350;
    static constexpr int kMinHeight = 250;
};
