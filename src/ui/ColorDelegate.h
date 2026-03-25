#pragma once

#include <QStyledItemDelegate>

class ColorDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit ColorDelegate(QObject* parent = nullptr);

    void paint(QPainter* painter,
        const QStyleOptionViewItem& option,
        const QModelIndex& index) const override;
};
