#ifndef CENTERALIGNEDDELEGATE_H
#define CENTERALIGNEDDELEGATE_H

#include <QStyledItemDelegate>
#include <QEvent>
#include <QMouseEvent>
#include <QStyleOptionButton>
#include <QApplication>
#include <QPainter>
#include <QTableWidget>
#include <QColor>

class CenterAlignedDelegate : public QStyledItemDelegate 
{
public:
    explicit CenterAlignedDelegate(QObject *parent = nullptr);
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

class LockRowDelegate : public QStyledItemDelegate
{
public:
    explicit LockRowDelegate(int stateColumn, QObject *parent = nullptr)
        : QStyledItemDelegate(parent),
          m_stateColumn(stateColumn)
    {
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    int m_stateColumn;
};


#endif // CENTERALIGNEDDELEGATE_H
