#include "ColorDelegate.h"
#include <QPainter>

ColorDelegate::ColorDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{}

void ColorDelegate::paint(
    QPainter* painter,
    const QStyleOptionViewItem& option,
    const QModelIndex& index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    bool selected = opt.state & QStyle::State_Selected;

    if (selected) {
        opt.palette.setColor(QPalette::HighlightedText, Qt::white);
        opt.font.setBold(true);
    } else {
        QVariant fg = index.data(Qt::ForegroundRole);
        if (fg.isValid() && fg.canConvert<QColor>()) {
            opt.palette.setColor(
                QPalette::Text, fg.value<QColor>());
        }
    }

    QStyledItemDelegate::paint(painter, opt, index);
}
