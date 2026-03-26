#pragma once

#include "models/Trade.h"
#include <QWidget>
#include <memory>

class PlChartWidget : public QWidget {
    Q_OBJECT
public:
    struct Data {
        double underlyingPrice = 0.0;
        double standardDeviation = 0.0;
        int dte = 0;
        double plMultiplier = 1.0;
        QString ticker;
        std::shared_ptr<Trade> trade;
    };

    explicit PlChartWidget(QWidget* parent = nullptr);

    void setData(const Data& data);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    struct ChartMetrics {
        double chartLeft, chartTop, chartWidth, chartHeight;
        double yMin, yMax;
    };

    ChartMetrics calcMetrics() const;
    double priceAtX(double px, const ChartMetrics& m) const;
    double xAtPrice(double price, const ChartMetrics& m) const;
    double yAtPl(double pl, const ChartMetrics& m) const;

    void drawGridLines(QPainter& p, const ChartMetrics& m);
    void drawGridLabels(QPainter& p, const ChartMetrics& m);
    void drawCurrentPriceLine(QPainter& p, const ChartMetrics& m);
    void drawShading(QPainter& p, const ChartMetrics& m);
    void drawPlLine(QPainter& p, const ChartMetrics& m);
    void drawCurrentPlLine(QPainter& p, const ChartMetrics& m);
    void drawBreakevenDots(QPainter& p, const ChartMetrics& m);
    void drawTooltip(QPainter& p, const ChartMetrics& m);

    static double niceInterval(double range, int targetTicks);
    void clampView();

    Data m_data;
    double m_viewLeft = 0.0;
    double m_viewRight = 0.0;

    bool m_panning = false;
    QPointF m_panStart;
    double m_panViewLeftStart = 0.0;

    QPointF m_mousePos;
    bool m_mouseInChart = false;

    static constexpr int kLeftMargin = 80;
    static constexpr int kRightMargin = 20;
    static constexpr int kTopMargin = 15;
    static constexpr int kBottomMargin = 35;

    static const QColor kPositiveColor;
    static const QColor kNegativeColor;
    static const QColor kCurrentPlColor;
    static const QColor kGridColor;
    static const QColor kBgColor;
};
