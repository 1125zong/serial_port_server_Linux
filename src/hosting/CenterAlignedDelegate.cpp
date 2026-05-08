#include "CenterAlignedDelegate.h"
#include <QPainter>

CenterAlignedDelegate::CenterAlignedDelegate(QObject *parent) : QStyledItemDelegate(parent) {}

void CenterAlignedDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const {
    QStyleOptionViewItem opt = option;
    opt.displayAlignment = Qt::AlignCenter; // 设置文本居中对齐
    QStyledItemDelegate::paint(painter, opt, index);
}

