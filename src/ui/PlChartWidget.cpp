#include "PlChartWidget.h"
#include "core/BlackScholes.h"
#include "core/MathUtils.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QLocale>
#include <cmath>
#include <algorithm>

const QColor PlChartWidget::kPositiveColor(0x00, 0xFF, 0x98);
const QColor PlChartWidget::kNegativeColor(0xC4, 0x1E, 0x3A);
const QColor PlChartWidget::kCurrentPlColor(0x00, 0x3C, 0xCC);
const QColor PlChartWidget::kGridColor(60, 60, 60);
const QColor PlChartWidget::kBgColor(20, 20, 20);

PlChartWidget::PlChartWidget(QWidget* parent)
    : QWidget(parent)
{
    setMouseTracking(true);
}

void PlChartWidget::setData(const Data& data) {
    m_data = data;
    m_viewLeft = data.underlyingPrice * 0.9;
    m_viewRight = data.underlyingPrice * 1.1;
    update();
}

void PlChartWidget::clampView() {
    double minPrice = m_data.underlyingPrice * 0.7;
    double maxPrice = m_data.underlyingPrice * 1.3;
    double viewWidth = m_viewRight - m_viewLeft;

    if (m_viewLeft < minPrice) {
        m_viewLeft = minPrice;
        m_viewRight = minPrice + viewWidth;
    }
    if (m_viewRight > maxPrice) {
        m_viewRight = maxPrice;
        m_viewLeft = maxPrice - viewWidth;
    }
    if (m_viewLeft < minPrice) m_viewLeft = minPrice;
}

PlChartWidget::ChartMetrics PlChartWidget::calcMetrics() const {
    ChartMetrics m;
    m.chartLeft = kLeftMargin;
    m.chartTop = kTopMargin;
    m.chartWidth = width() - kLeftMargin - kRightMargin;
    m.chartHeight = height() - kTopMargin - kBottomMargin;

    if (m.chartWidth <= 0 || m.chartHeight <= 0 || !m_data.trade)
        return m;

    // Sample P/L across visible range to find Y bounds
    double yMin = 0.0, yMax = 0.0;
    int samples = std::max(static_cast<int>(m.chartWidth), 100);
    for (int i = 0; i <= samples; ++i) {
        double price = m_viewLeft
            + (m_viewRight - m_viewLeft) * i / samples;
        double pl = m_data.trade->profitLossAtPrice(price)
            * m_data.plMultiplier;
        double currentPl = BlackScholes::currentPlAtPrice(
            *m_data.trade, price, m_data.dte)
            * m_data.plMultiplier;
        double maxVal = std::max(pl, currentPl);
        double minVal = std::min(pl, currentPl);
        if (i == 0 || maxVal > yMax) yMax = maxVal;
        if (i == 0 || minVal < yMin) yMin = minVal;
    }

    // Ensure zero line is always visible
    if (yMin > 0.0) yMin = 0.0;
    if (yMax < 0.0) yMax = 0.0;

    // Pad Y range by 10%
    double yRange = yMax - yMin;
    if (yRange < 0.01) yRange = 1.0;
    m.yMin = yMin - yRange * 0.1;
    m.yMax = yMax + yRange * 0.1;

    return m;
}

double PlChartWidget::priceAtX(double px,
    const ChartMetrics& m) const
{
    double ratio = (px - m.chartLeft) / m.chartWidth;
    return m_viewLeft + ratio * (m_viewRight - m_viewLeft);
}

double PlChartWidget::xAtPrice(double price,
    const ChartMetrics& m) const
{
    double ratio = (price - m_viewLeft)
        / (m_viewRight - m_viewLeft);
    return m.chartLeft + ratio * m.chartWidth;
}

double PlChartWidget::yAtPl(double pl,
    const ChartMetrics& m) const
{
    double ratio = (m.yMax - pl) / (m.yMax - m.yMin);
    return m.chartTop + ratio * m.chartHeight;
}

double PlChartWidget::niceInterval(double range,
    int targetTicks)
{
    if (range <= 0 || targetTicks <= 0) return 1.0;
    double rough = range / targetTicks;
    double mag = std::pow(10.0, std::floor(std::log10(rough)));
    double residual = rough / mag;
    double nice;
    if (residual <= 1.5) nice = 1.0;
    else if (residual <= 3.0) nice = 2.0;
    else if (residual <= 7.0) nice = 5.0;
    else nice = 10.0;
    return nice * mag;
}

void PlChartWidget::drawGridLines(QPainter& p,
    const ChartMetrics& m)
{
    // X axis grid lines
    double xInterval = niceInterval(
        m_viewRight - m_viewLeft, 8);
    double xStart = std::ceil(m_viewLeft / xInterval) * xInterval;
    QPen gridPen(kGridColor, 1, Qt::SolidLine);

    for (double price = xStart; price <= m_viewRight;
         price += xInterval)
    {
        double x = xAtPrice(price, m);
        p.setPen(gridPen);
        p.drawLine(QPointF(x, m.chartTop),
                   QPointF(x, m.chartTop + m.chartHeight));
    }

    // Y axis grid lines
    double yInterval = niceInterval(m.yMax - m.yMin, 6);
    double yStart = std::ceil(m.yMin / yInterval) * yInterval;

    for (double pl = yStart; pl <= m.yMax; pl += yInterval) {
        double y = yAtPl(pl, m);
        bool isZero = std::abs(pl) < yInterval * 0.01;

        if (isZero) {
            p.setPen(QPen(QColor(120, 120, 120), 2));
        } else {
            p.setPen(gridPen);
        }
        p.drawLine(QPointF(m.chartLeft, y),
                   QPointF(m.chartLeft + m.chartWidth, y));
    }
}

void PlChartWidget::drawGridLabels(QPainter& p,
    const ChartMetrics& m)
{
    QFont font("Segoe UI", 8);
    p.setFont(font);
    QLocale locale(QLocale::English);

    // X axis labels
    double xInterval = niceInterval(
        m_viewRight - m_viewLeft, 8);
    double xStart = std::ceil(m_viewLeft / xInterval) * xInterval;

    for (double price = xStart; price <= m_viewRight;
         price += xInterval)
    {
        double x = xAtPrice(price, m);
        p.setPen(QColor(180, 180, 180));
        QString label = locale.toString(price, 'f',
            price >= 100.0 ? 0 : 2);
        p.drawText(QRectF(x - 40, m.chartTop + m.chartHeight + 4,
                          80, 20),
                   Qt::AlignHCenter | Qt::AlignTop, label);
    }

    // Y axis labels
    double yInterval = niceInterval(m.yMax - m.yMin, 6);
    double yStart = std::ceil(m.yMin / yInterval) * yInterval;

    for (double pl = yStart; pl <= m.yMax; pl += yInterval) {
        double y = yAtPl(pl, m);

        QColor labelColor = (pl > 0.01) ? kPositiveColor
            : (pl < -0.01) ? kNegativeColor
            : QColor(180, 180, 180);
        p.setPen(labelColor);

        QString label;
        double absVal = std::abs(pl);
        if (absVal >= 1000.0)
            label = locale.toString(qRound64(pl));
        else
            label = QString::number(pl, 'f', 2);

        p.drawText(QRectF(2, y - 10, kLeftMargin - 6, 20),
                   Qt::AlignRight | Qt::AlignVCenter, label);
    }
}

void PlChartWidget::drawCurrentPriceLine(QPainter& p,
    const ChartMetrics& m)
{
    double x = xAtPrice(m_data.underlyingPrice, m);
    if (x < m.chartLeft || x > m.chartLeft + m.chartWidth) return;

    QPen pen(QColor(140, 140, 140), 1, Qt::DashLine);
    p.setPen(pen);
    p.drawLine(QPointF(x, m.chartTop),
               QPointF(x, m.chartTop + m.chartHeight));
}

void PlChartWidget::drawShading(QPainter& p,
    const ChartMetrics& m)
{
    if (!m_data.trade) return;

    double zeroY = yAtPl(0.0, m);
    int pixelCount = static_cast<int>(m.chartWidth);
    if (pixelCount <= 0) return;

    // Build positive and negative paths
    QPainterPath posPath, negPath;
    bool posStarted = false, negStarted = false;

    for (int i = 0; i <= pixelCount; ++i) {
        double px = m.chartLeft + i;
        double price = priceAtX(px, m);
        double pl = m_data.trade->profitLossAtPrice(price)
            * m_data.plMultiplier;
        double plY = yAtPl(pl, m);

        if (pl >= 0) {
            if (!posStarted) {
                posPath.moveTo(px, zeroY);
                posStarted = true;
            }
            posPath.lineTo(px, plY);

            if (negStarted) {
                negPath.lineTo(px, zeroY);
                negStarted = false;
            }
        } else {
            if (!negStarted) {
                negPath.moveTo(px, zeroY);
                negStarted = true;
            }
            negPath.lineTo(px, plY);

            if (posStarted) {
                posPath.lineTo(px, zeroY);
                posStarted = false;
            }
        }
    }
    if (posStarted) {
        posPath.lineTo(m.chartLeft + pixelCount, zeroY);
    }
    if (negStarted) {
        negPath.lineTo(m.chartLeft + pixelCount, zeroY);
    }

    QColor posShade = kPositiveColor;
    posShade.setAlpha(30);
    QColor negShade = kNegativeColor;
    negShade.setAlpha(30);

    p.setPen(Qt::NoPen);
    p.setBrush(posShade);
    p.drawPath(posPath);
    p.setBrush(negShade);
    p.drawPath(negPath);
}

void PlChartWidget::drawPlLine(QPainter& p,
    const ChartMetrics& m)
{
    if (!m_data.trade) return;

    int pixelCount = static_cast<int>(m.chartWidth);
    if (pixelCount <= 0) return;

    QPen posPen(kPositiveColor, 3);
    QPen negPen(kNegativeColor, 3);

    double prevPx = m.chartLeft;
    double prevPrice = priceAtX(prevPx, m);
    double prevPl = m_data.trade->profitLossAtPrice(prevPrice)
        * m_data.plMultiplier;
    double prevY = yAtPl(prevPl, m);

    for (int i = 1; i <= pixelCount; ++i) {
        double px = m.chartLeft + i;
        double price = priceAtX(px, m);
        double pl = m_data.trade->profitLossAtPrice(price)
            * m_data.plMultiplier;
        double curY = yAtPl(pl, m);

        // Check for zero crossing between prev and current
        if ((prevPl >= 0) != (pl >= 0) && prevPl != pl) {
            double t = prevPl / (prevPl - pl);
            double crossX = prevPx + t * (px - prevPx);
            double zeroY = yAtPl(0.0, m);

            p.setPen(prevPl >= 0 ? posPen : negPen);
            p.drawLine(QPointF(prevPx, prevY),
                       QPointF(crossX, zeroY));
            p.setPen(pl >= 0 ? posPen : negPen);
            p.drawLine(QPointF(crossX, zeroY),
                       QPointF(px, curY));
        } else {
            p.setPen(pl >= 0 ? posPen : negPen);
            p.drawLine(QPointF(prevPx, prevY),
                       QPointF(px, curY));
        }

        prevPx = px;
        prevPl = pl;
        prevY = curY;
    }
}

void PlChartWidget::drawCurrentPlLine(QPainter& p,
    const ChartMetrics& m)
{
    if (!m_data.trade || m_data.dte <= 0) return;

    int pixelCount = static_cast<int>(m.chartWidth);
    if (pixelCount <= 0) return;

    QColor curPlColor = kCurrentPlColor;
    curPlColor.setAlpha(128);
    QPen pen(curPlColor, 2);
    p.setPen(pen);

    double prevPx = m.chartLeft;
    double prevPrice = priceAtX(prevPx, m);
    double prevPl = BlackScholes::currentPlAtPrice(
        *m_data.trade, prevPrice, m_data.dte)
        * m_data.plMultiplier;
    double prevY = yAtPl(prevPl, m);

    for (int i = 1; i <= pixelCount; ++i) {
        double px = m.chartLeft + i;
        double price = priceAtX(px, m);
        double pl = BlackScholes::currentPlAtPrice(
            *m_data.trade, price, m_data.dte)
            * m_data.plMultiplier;
        double curY = yAtPl(pl, m);

        p.drawLine(QPointF(prevPx, prevY),
                   QPointF(px, curY));

        prevPx = px;
        prevPl = pl;
        prevY = curY;
    }
}

void PlChartWidget::drawBreakevenDots(QPainter& p,
    const ChartMetrics& m)
{
    if (!m_data.trade) return;

    int pixelCount = static_cast<int>(m.chartWidth);
    if (pixelCount <= 0) return;

    double prevPrice = priceAtX(m.chartLeft, m);
    double prevPl = m_data.trade->profitLossAtPrice(prevPrice)
        * m_data.plMultiplier;

    p.setPen(Qt::NoPen);
    p.setBrush(QColor(160, 160, 160));

    for (int i = 1; i <= pixelCount; ++i) {
        double px = m.chartLeft + i;
        double price = priceAtX(px, m);
        double pl = m_data.trade->profitLossAtPrice(price)
            * m_data.plMultiplier;

        if ((prevPl >= 0) != (pl >= 0) && prevPl != pl) {
            double t = prevPl / (prevPl - pl);
            double crossX = (m.chartLeft + i - 1) + t;
            double zeroY = yAtPl(0.0, m);
            p.drawEllipse(QPointF(crossX, zeroY), 5, 5);
        }
        prevPl = pl;
    }
}

void PlChartWidget::drawTooltip(QPainter& p,
    const ChartMetrics& m)
{
    if (!m_mouseInChart || !m_data.trade) return;

    double mx = m_mousePos.x();
    double my = m_mousePos.y();

    if (mx < m.chartLeft
        || mx > m.chartLeft + m.chartWidth
        || my < m.chartTop
        || my > m.chartTop + m.chartHeight)
        return;

    double price = priceAtX(mx, m);
    double expirationPl = m_data.trade->profitLossAtPrice(price)
        * m_data.plMultiplier;
    double currentPl = BlackScholes::currentPlAtPrice(
        *m_data.trade, price, m_data.dte)
        * m_data.plMultiplier;

    double sigmaRootT = m_data.standardDeviation / m_data.underlyingPrice;
    double T = static_cast<double>(m_data.dte) / 365.0;
    double probBelow = MathUtils::lognormalCdf(
        price, m_data.underlyingPrice, sigmaRootT, T) * 100.0;
    double probAbove = 100.0 - probBelow;

    QLocale locale(QLocale::English);
    QStringList lines;
    QString priceLabel = m_data.ticker.isEmpty()
        ? "Price" : m_data.ticker;
    lines << QString("%1: %2")
        .arg(priceLabel, locale.toString(price, 'f', 2));
    lines << QString("Exp P/L: %1")
        .arg(locale.toString(qRound64(expirationPl)));
    if (m_data.dte > 0) {
        lines << QString("Cur P/L: %1")
            .arg(locale.toString(qRound64(currentPl)));
    }
    lines << QString("\u2190 %1%  \u00B7  %2% \u2192")
        .arg(QString::number(probBelow, 'f', 1),
             QString::number(probAbove, 'f', 1));

    QFont font("Segoe UI", 9);
    p.setFont(font);
    QFontMetrics fm(font);

    int lineHeight = fm.height() + 2;
    int tooltipWidth = 0;
    for (const auto& line : lines)
        tooltipWidth = std::max(tooltipWidth,
            fm.horizontalAdvance(line));
    tooltipWidth += 16;
    int tooltipHeight = lineHeight * lines.size() + 8;

    double tx = mx + 15;
    double ty = my - tooltipHeight - 5;
    if (tx + tooltipWidth > m.chartLeft + m.chartWidth)
        tx = mx - tooltipWidth - 15;
    if (ty < m.chartTop)
        ty = my + 15;

    p.setPen(Qt::NoPen);
    QColor tooltipBg(60, 60, 60, 220);
    p.setBrush(tooltipBg);
    p.drawRoundedRect(QRectF(tx, ty, tooltipWidth,
                             tooltipHeight), 4, 4);

    p.setPen(QColor(220, 220, 220));
    for (int i = 0; i < lines.size(); ++i) {
        p.drawText(QRectF(tx + 8, ty + 4 + i * lineHeight,
                          tooltipWidth - 16, lineHeight),
                   Qt::AlignLeft | Qt::AlignVCenter,
                   lines[i]);
    }
}

void PlChartWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Background
    p.fillRect(rect(), kBgColor);

    if (!m_data.trade || m_viewRight <= m_viewLeft) return;

    auto m = calcMetrics();
    if (m.chartWidth <= 0 || m.chartHeight <= 0) return;

    // Clip to chart area for drawing
    p.save();
    p.setClipRect(QRectF(m.chartLeft, m.chartTop,
                         m.chartWidth, m.chartHeight));

    drawGridLines(p, m);
    drawShading(p, m);
    drawCurrentPriceLine(p, m);
    drawCurrentPlLine(p, m);
    drawPlLine(p, m);
    drawBreakevenDots(p, m);

    p.restore();

    // Labels drawn without clipping (they sit outside chart area)
    drawGridLabels(p, m);
    drawTooltip(p, m);
}

void PlChartWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        auto m = calcMetrics();
        bool inGrid = event->position().x() >= m.chartLeft
            && event->position().x() <= m.chartLeft + m.chartWidth
            && event->position().y() >= m.chartTop
            && event->position().y() <= m.chartTop + m.chartHeight;
        if (inGrid) {
            m_panning = true;
            m_panStart = event->position();
            m_panViewLeftStart = m_viewLeft;
            setCursor(Qt::ClosedHandCursor);
        }
    }
}

void PlChartWidget::mouseMoveEvent(QMouseEvent* event) {
    m_mousePos = event->position();
    m_mouseInChart = true;

    if (m_panning) {
        auto m = calcMetrics();
        if (m.chartWidth <= 0) return;
        double viewWidth = m_viewRight - m_viewLeft;
        double dx = event->position().x() - m_panStart.x();
        double pricePerPixel = viewWidth / m.chartWidth;
        double priceDelta = -dx * pricePerPixel;
        m_viewLeft = m_panViewLeftStart + priceDelta;
        m_viewRight = m_viewLeft + viewWidth;
        clampView();
    } else {
        // Crosshair only inside chart grid area
        auto m = calcMetrics();
        bool inGrid = m_mousePos.x() >= m.chartLeft
            && m_mousePos.x() <= m.chartLeft + m.chartWidth
            && m_mousePos.y() >= m.chartTop
            && m_mousePos.y() <= m.chartTop + m.chartHeight;
        setCursor(inGrid ? Qt::CrossCursor : Qt::ArrowCursor);
    }
    update();
}

void PlChartWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && m_panning) {
        m_panning = false;
        // Restore cursor based on position
        auto m = calcMetrics();
        bool inGrid = m_mousePos.x() >= m.chartLeft
            && m_mousePos.x() <= m.chartLeft + m.chartWidth
            && m_mousePos.y() >= m.chartTop
            && m_mousePos.y() <= m.chartTop + m.chartHeight;
        setCursor(inGrid ? Qt::CrossCursor : Qt::ArrowCursor);
    }
}

void PlChartWidget::wheelEvent(QWheelEvent* event) {
    double zoomFactor = (event->angleDelta().y() > 0)
        ? 0.85 : 1.15;

    auto m = calcMetrics();
    double mousePrice = priceAtX(event->position().x(), m);
    double ratioLeft = (mousePrice - m_viewLeft)
        / (m_viewRight - m_viewLeft);

    double newWidth =
        (m_viewRight - m_viewLeft) * zoomFactor;

    // Clamp minimum zoom to ~2% range
    double minWidth = m_data.underlyingPrice * 0.02;
    // Clamp maximum zoom to 60% range
    double maxWidth = m_data.underlyingPrice * 0.6;
    newWidth = std::clamp(newWidth, minWidth, maxWidth);

    m_viewLeft = mousePrice - ratioLeft * newWidth;
    m_viewRight = m_viewLeft + newWidth;
    clampView();
    update();

    event->accept();
}

void PlChartWidget::leaveEvent(QEvent*) {
    m_mouseInChart = false;
    update();
}
