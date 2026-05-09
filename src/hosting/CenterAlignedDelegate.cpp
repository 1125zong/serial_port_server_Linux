#include "CenterAlignedDelegate.h"
#include <QPainter>

CenterAlignedDelegate::CenterAlignedDelegate(QObject *parent) : QStyledItemDelegate(parent) {}

void CenterAlignedDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    opt.displayAlignment = Qt::AlignCenter; // 设置文本居中对齐
    QStyledItemDelegate::paint(painter, opt, index);
}

void LockRowDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    opt.displayAlignment = Qt::AlignCenter;

    QString state = index.sibling(index.row(), m_stateColumn)
                        .data(Qt::DisplayRole)
                        .toString()
                        .trimmed();

    if (state == "已锁定" || state == "锁定")
    {
        QColor bg = (opt.state & QStyle::State_Selected) ? QColor("#7a3333") : QColor("#5a2a2a");

        painter->save();
        painter->fillRect(opt.rect, bg);
        painter->restore();

        opt.backgroundBrush = QBrush(bg);
        opt.palette.setColor(QPalette::Text, Qt::white);
        opt.palette.setColor(QPalette::HighlightedText, Qt::white);
    }

    QStyledItemDelegate::paint(painter, opt, index);
}
