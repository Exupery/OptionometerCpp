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

    QVariant fg = index.data(Qt::ForegroundRole);
    if (fg.isValid() && fg.canConvert<QColor>()) {
        opt.palette.setColor(QPalette::Text, fg.value<QColor>());
    }

    QStyledItemDelegate::paint(painter, opt, index);
}
