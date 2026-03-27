#include "ChartWindow.h"
#include <QPainter>
#include <QMouseEvent>
#include <QApplication>

QSize ChartWindow::s_lastSize;

ChartWindow::ChartWindow(const QString& title,
                         const PlChartWidget::Data& data,
                         QWidget* parent,
                         const QSize& savedSize)
    : QWidget(parent)
{
    // Split title: "TICKER  price" on left, trade on right
    // Title format is "TICKER price TRADE_STRING"
    // Find first space after ticker, then second space after price
    int firstSpace = title.indexOf(' ');
    if (firstSpace >= 0) {
        int secondSpace = title.indexOf(' ', firstSpace + 1);
        if (secondSpace >= 0) {
            m_tickerPrice = title.left(secondSpace);
            m_tradeText = title.mid(secondSpace + 1);
        } else {
            m_tickerPrice = title;
        }
    } else {
        m_tickerPrice = title;
    }
    // Insert extra space between ticker and price
    m_tickerPrice.replace(m_tickerPrice.indexOf(' '), 1,
                          "  ");
    setMouseTracking(true);

    m_chart = new PlChartWidget(this);
    m_chart->setData(data);
    m_chart->move(1, kTitleBarHeight);
    m_chart->installEventFilter(this);

    // Size: use last used size, then saved size, then 1/4 of parent
    QSize sz;
    if (s_lastSize.isValid()) {
        sz = s_lastSize;
    } else if (savedSize.isValid()) {
        sz = savedSize;
        s_lastSize = sz;
    } else if (parent) {
        sz = QSize(parent->width() / 2, parent->height() / 2);
    } else {
        sz = QSize(500, 350);
    }
    sz = sz.expandedTo(QSize(kMinWidth, kMinHeight));
    resize(sz);

    // Center in parent
    if (parent) {
        move((parent->width() - width()) / 2,
             (parent->height() - height()) / 2);
    }

    show();
    raise();
}

QSize ChartWindow::lastUsedSize() {
    return s_lastSize;
}

void ChartWindow::setShowDteHalf(bool show) {
    m_chart->setShowDteHalf(show);
}

void ChartWindow::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Border
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0x00, 0x3C, 0xCC));
    p.drawRect(0, 0, width(), height());

    // Title bar
    p.fillRect(0, 0, width(), kTitleBarHeight,
               QColor(0x00, 0x3C, 0xCC));

    // Title text - ticker+price left, trade right
    QFont font("Segoe UI", 9);
    font.setBold(true);
    p.setFont(font);
    p.setPen(QColor(220, 220, 220));

    int textAreaWidth = width() - 36;
    p.drawText(QRect(8, 0, textAreaWidth, kTitleBarHeight),
               Qt::AlignLeft | Qt::AlignVCenter, m_tickerPrice);
    if (!m_tradeText.isEmpty()) {
        p.drawText(QRect(8, 0, textAreaWidth, kTitleBarHeight),
                   Qt::AlignRight | Qt::AlignVCenter, m_tradeText);
    }

    // Close button
    int btnSize = 18;
    int btnX = width() - btnSize - 6;
    int btnY = (kTitleBarHeight - btnSize) / 2;
    m_closeButtonRect = QRect(btnX, btnY, btnSize, btnSize);

    p.setPen(QPen(QColor(180, 180, 180), 2));
    int pad = 4;
    p.drawLine(btnX + pad, btnY + pad,
               btnX + btnSize - pad, btnY + btnSize - pad);
    p.drawLine(btnX + btnSize - pad, btnY + pad,
               btnX + pad, btnY + btnSize - pad);

    // Resize the chart widget to fill the body, leaving border
    m_chart->setGeometry(kBorderSize, kTitleBarHeight,
                         width() - 2 * kBorderSize,
                         height() - kTitleBarHeight - kBorderSize);
}

ChartWindow::HitZone ChartWindow::hitTest(
    const QPoint& pos) const
{
    if (pos.y() < kTitleBarHeight) return TitleBar;

    // Triangle-shaped resize zones in bottom corners
    // Bottom-left: point at (kResizeGrabSize, h-kResizeGrabSize),
    //   extending to (0, h-kResizeGrabSize) and (0, h) and
    //   (kResizeGrabSize, h)
    // Test: distance from bottom + distance from side <= grab size
    int distFromBottom = height() - pos.y();
    int distFromLeft = pos.x();
    int distFromRight = width() - pos.x();

    if (distFromBottom <= kResizeGrabSize
        && distFromLeft <= kResizeGrabSize
        && (distFromBottom + distFromLeft) <= kResizeGrabSize)
        return BottomLeft;

    if (distFromBottom <= kResizeGrabSize
        && distFromRight <= kResizeGrabSize
        && (distFromBottom + distFromRight) <= kResizeGrabSize)
        return BottomRight;

    return None;
}

void ChartWindow::updateCursorShape(const QPoint& pos) {
    auto zone = hitTest(pos);
    switch (zone) {
    case BottomLeft:
        setCursor(Qt::SizeBDiagCursor); break;
    case BottomRight:
        setCursor(Qt::SizeFDiagCursor); break;
    case TitleBar:
        setCursor(Qt::ArrowCursor); break;
    default:
        setCursor(Qt::ArrowCursor); break;
    }
}

void ChartWindow::mousePressEvent(QMouseEvent* event) {
    if (event->button() != Qt::LeftButton) return;

    raise();

    // Close button
    if (m_closeButtonRect.contains(event->pos())) {
        emit closed();
        deleteLater();
        return;
    }

    m_activeZone = hitTest(event->pos());
    if (m_activeZone != None) {
        m_dragStartGlobal = event->globalPosition().toPoint();
        m_windowStartPos = pos();
        m_windowStartSize = size();
    }
}

void ChartWindow::mouseMoveEvent(QMouseEvent* event) {
    if (m_activeZone == None) {
        updateCursorShape(event->pos());
        return;
    }

    QPoint delta = event->globalPosition().toPoint()
        - m_dragStartGlobal;

    if (m_activeZone == TitleBar) {
        QPoint newPos = m_windowStartPos + delta;
        // Keep within parent bounds
        if (parentWidget()) {
            int maxX = parentWidget()->width() - 50;
            int maxY = parentWidget()->height() - 50;
            newPos.setX(std::clamp(newPos.x(), -width() + 50,
                                   maxX));
            newPos.setY(std::clamp(newPos.y(), 0, maxY));
        }
        move(newPos);
    } else if (m_activeZone == BottomRight) {
        int newW = std::max(m_windowStartSize.width() + delta.x(),
                            kMinWidth);
        int newH = std::max(m_windowStartSize.height() + delta.y(),
                            kMinHeight);
        resize(newW, newH);
        s_lastSize = size();
        emit sizeChanged(size());
    } else if (m_activeZone == BottomLeft) {
        int newW = std::max(m_windowStartSize.width() - delta.x(),
                            kMinWidth);
        int newH = std::max(m_windowStartSize.height() + delta.y(),
                            kMinHeight);
        int newX = m_windowStartPos.x()
            + (m_windowStartSize.width() - newW);
        resize(newW, newH);
        move(newX, m_windowStartPos.y());
        s_lastSize = size();
        emit sizeChanged(size());
    }

    update();
}

void ChartWindow::mouseReleaseEvent(QMouseEvent*) {
    m_activeZone = None;
}

bool ChartWindow::eventFilter(QObject* obj, QEvent* event) {
    if (obj != m_chart)
        return QWidget::eventFilter(obj, event);

    auto mapToWindow = [this](QMouseEvent* me) {
        return m_chart->mapToParent(me->pos());
    };

    if (event->type() == QEvent::MouseMove) {
        auto* me = static_cast<QMouseEvent*>(event);
        QPoint wpos = mapToWindow(me);

        if (m_activeZone == BottomLeft
            || m_activeZone == BottomRight) {
            // Forward resize drag to our handler
            QMouseEvent mapped(me->type(),
                QPointF(wpos), me->globalPosition(),
                me->button(), me->buttons(), me->modifiers());
            mouseMoveEvent(&mapped);
            return true;
        }

        // Update cursor for resize zones
        auto zone = hitTest(wpos);
        if (zone == BottomLeft) {
            m_chart->setCursor(Qt::SizeBDiagCursor);
            return false;
        } else if (zone == BottomRight) {
            m_chart->setCursor(Qt::SizeFDiagCursor);
            return false;
        } else {
            m_chart->unsetCursor();
            return false;
        }
    }

    if (event->type() == QEvent::MouseButtonPress) {
        auto* me = static_cast<QMouseEvent*>(event);
        if (me->button() != Qt::LeftButton)
            return false;

        QPoint wpos = mapToWindow(me);
        auto zone = hitTest(wpos);
        if (zone == BottomLeft || zone == BottomRight) {
            raise();
            m_activeZone = zone;
            m_dragStartGlobal =
                me->globalPosition().toPoint();
            m_windowStartPos = pos();
            m_windowStartSize = size();
            return true;
        }
        return false;
    }

    if (event->type() == QEvent::MouseButtonRelease) {
        if (m_activeZone == BottomLeft
            || m_activeZone == BottomRight) {
            m_activeZone = None;
            return true;
        }
        return false;
    }

    return QWidget::eventFilter(obj, event);
}
