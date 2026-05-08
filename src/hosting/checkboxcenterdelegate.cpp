#include "checkboxcenterdelegate.h"

CheckBoxCenterDelegate::CheckBoxCenterDelegate(QObject *parent, const QSize &indicatorSize)
    : QStyledItemDelegate(parent),
      m_indicatorSize(indicatorSize)
{

}

// 重写 paint 方法，自己绘制 checkbox，并让它始终位于单元格中心
void CheckBoxCenterDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QVariant checkData = index.data(Qt::CheckStateRole);    // 获取复选框状态数据

    // 如果这个单元格没有复选框，使用默认绘制
    if (!checkData.isValid())
    {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    QStyleOptionViewItem opt(option);
    initStyleOption(&opt, index);

    QStyle *style = opt.widget ? opt.widget->style() : QApplication::style();   // 

    /*
     * 先绘制单元格背景、选中效果等，
     * 但去掉 Qt 默认的 checkbox 绘制，避免出现两个 checkbox
     */
    QStyleOptionViewItem bgOpt(opt);
    bgOpt.features &= ~QStyleOptionViewItem::HasCheckIndicator;
    bgOpt.text.clear();
    bgOpt.icon = QIcon();
    bgOpt.state &= ~QStyle::State_HasFocus;

    style->drawControl(QStyle::CE_ItemViewItem,
                       &bgOpt,
                       painter,
                       opt.widget);

    /*
     * 自己计算 checkbox 的位置
     * 让它始终位于单元格中心
     */
    QRect checkRect(QPoint(0, 0), m_indicatorSize);
    checkRect.moveCenter(option.rect.center());

    QStyleOptionViewItem checkOpt(opt);
    checkOpt.rect = checkRect;

    checkOpt.state &= ~(QStyle::State_On |
                        QStyle::State_Off |
                        QStyle::State_NoChange);

    Qt::CheckState state =
            static_cast<Qt::CheckState>(checkData.toInt());

    if (state == Qt::Checked)
    {
        checkOpt.state |= QStyle::State_On;
    }
    else if (state == Qt::PartiallyChecked)
    {
        checkOpt.state |= QStyle::State_NoChange;
    }
    else
    {
        checkOpt.state |= QStyle::State_Off;
    }

    style->drawPrimitive(QStyle::PE_IndicatorItemViewItemCheck,
                         &checkOpt,
                         painter,
                         opt.widget);
}

bool CheckBoxCenterDelegate::editorEvent(QEvent *event,
                                         QAbstractItemModel *model,
                                         const QStyleOptionViewItem &option,
                                         const QModelIndex &index)
{
    if (!index.data(Qt::CheckStateRole).isValid())
    {
        return QStyledItemDelegate::editorEvent(event, model, option, index);
    }

    if (!(index.flags() & Qt::ItemIsUserCheckable) ||
            !(index.flags() & Qt::ItemIsEnabled))
    {
        return false;
    }

    // 鼠标点击切换 checkbox 状态
    if (event->type() == QEvent::MouseButtonRelease)
    {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);

        if (mouseEvent->button() != Qt::LeftButton)
        {
            return false;
        }

        // 当前写法：点击这个单元格任意位置都可以切换
        if (!option.rect.contains(mouseEvent->pos()))
        {
            return false;
        }

        return toggleCheckState(model, index);
    }

    // 键盘空格切换 checkbox 状态
    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);

        if (keyEvent->key() == Qt::Key_Space ||
                keyEvent->key() == Qt::Key_Select)
        {
            return toggleCheckState(model, index);
        }
    }

    return false;
}

bool CheckBoxCenterDelegate::toggleCheckState(QAbstractItemModel *model,
                                              const QModelIndex &index) const
{
    Qt::CheckState oldState =
            static_cast<Qt::CheckState>(index.data(Qt::CheckStateRole).toInt());

    Qt::CheckState newState =
            oldState == Qt::Checked ? Qt::Unchecked : Qt::Checked;

    return model->setData(index, newState, Qt::CheckStateRole);
}
