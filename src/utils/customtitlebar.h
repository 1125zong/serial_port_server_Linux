#ifndef CUSTOMTITLEBAR_H
#define CUSTOMTITLEBAR_H

#include <QWidget>
#include <QPoint> // 新增头文件

class QLabel;
class QPushButton;
class QHBoxLayout;
class CustomTitleBar : public QWidget
{
    Q_OBJECT
public:
    explicit CustomTitleBar(QWidget *parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    void initWidgets();       // 初始化控件
    void initConnections();   // 初始化信号槽

    // 控件指针
    QLabel *m_titleLabel;
    QPushButton *m_closeBtn;
    QPushButton *m_maxBtn;
    QPushButton *m_minBtn;
    QHBoxLayout *m_layout;

    // 颜色参数
    QColor m_titleBgColor;
    QColor m_titleTextColor;

    // 拖动偏移量（必须声明！）
    QPoint m_dragOffset;
};

#endif // CUSTOMTITLEBAR_H
