#ifndef CHECKBOXCENTERDELEGATE_H
#define CHECKBOXCENTERDELEGATE_H

#include <QStyledItemDelegate>
#include <QSize>
#include <QApplication>
#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QStyle>
#include <QStyleOptionViewItem>
#include <QAbstractItemModel>
#include <QIcon>

class CheckBoxCenterDelegate : public QStyledItemDelegate
{
public:
    explicit CheckBoxCenterDelegate(QObject *parent = nullptr, const QSize &indicatorSize = QSize(14, 14));

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index) override;

private:
    bool toggleCheckState(QAbstractItemModel *model, const QModelIndex &index) const;

private:
    QSize m_indicatorSize;
};

#endif // CHECKBOXCENTERDELEGATE_H
