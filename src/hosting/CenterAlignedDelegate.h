#ifndef CENTERALIGNEDDELEGATE_H
#define CENTERALIGNEDDELEGATE_H

#include <QStyledItemDelegate>
#include <QEvent>
#include <QMouseEvent>
#include <QStyleOptionButton>
#include <QApplication>
#include <QPainter>
#include <QTableWidget>

class CenterAlignedDelegate : public QStyledItemDelegate 
{
public:
    explicit CenterAlignedDelegate(QObject *parent = nullptr);
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};


#endif // CENTERALIGNEDDELEGATE_H
