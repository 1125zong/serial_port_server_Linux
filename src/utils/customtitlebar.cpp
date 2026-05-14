#include "customtitlebar.h"

// 引入实现所需头文件
#include <QPainter>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QMainWindow>
#include "Logger.h"

/**
 * @brief 构造函数实现
 * @param parent 父控件
 */
CustomTitleBar::CustomTitleBar(QWidget *parent)
    : QWidget(parent),
      m_titleLabel(nullptr),
      m_closeBtn(nullptr),
      m_maxBtn(nullptr),
      m_minBtn(nullptr),
      m_layout(nullptr),
      // 初始化默认颜色（深蓝色科技风）
      m_titleBgColor(10, 25, 41),    // #0a1929
      m_titleTextColor(192, 216, 240) // #c0d8f0
{
    Logger::instance()->log(Logger::Debug, "CustomTitleBar", "初始化标题栏");
    // ========== 2. 启用正确的控件/布局初始化 ==========
    initWidgets();
    initConnections();
    // ========== 3. 标题栏基础设置（关键：占满宽度+固定高度） ==========
    setMinimumHeight(40); // 固定高度，替代原来的32px（和按钮尺寸匹配）
    // 强制标题栏水平占满父布局，垂直固定高度（解决左上角白块核心）
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    // 设置背景透明
    setStyleSheet("background-color: transparent;");
    Logger::instance()->log(Logger::Debug, "CustomTitleBar", "标题栏初始化完成");
}


// 初始化标题栏内控件
void CustomTitleBar::initWidgets()
{
    Logger::instance()->log(Logger::Debug, "CustomTitleBar", "初始化标题栏控件");
    // 1. 创建标题标签
    m_titleLabel = new QLabel("设备查找", this);
    m_titleLabel->setFixedSize(60,20);
    // 设置默认文字颜色
    m_titleLabel->setStyleSheet(R"(
                                QLabel
                                {
                                color: rgb(192, 216, 240);
                                font-size: 14px;
                                }
                                )");

    // 2. 创建窗口控制按钮
    // 最小化按钮
    m_minBtn = new QPushButton(this);
    m_minBtn->setFixedSize(30, 30);
    m_minBtn->setToolTip("最小化");  // 鼠标悬停提示
    m_minBtn->setStyleSheet(R"(
                            QPushButton
                            {
                            border: none;
                            border-image: url(":/image/minimize.png");
                            background-color: transparent;
                            padding: 0px;
                            margin-right: 7px;
                            }
                            QPushButton:hover
                            {
                            border-image: url(":/image/minimizeHover.png");
                            }
                            QPushButton:pressed
                            {
                            outline: none;
                            }
                            QPushButton:focus
                            {
                            outline: none;
                            }
                            /* 设置提示词 */
                            QToolTip
                            {
                            background-color: rgba(255,255,255,0.5);  /* 背景颜色 */
                            color: balck;             /* 文字颜色 */
                            border: none;  /* 边框 */
                            font-size: 12px;            /* 字体大小 */
                            padding: 3px;               /* 内边距 */
                            border-radius: 3px;         /* 圆角 */
                            height: 10px;
                            }
                            )");

    // 最大化/还原按钮
    m_maxBtn = new QPushButton(this);
    m_maxBtn->setFixedSize(30, 30);
    m_maxBtn->setToolTip("放大窗口");  // 鼠标悬停提示
    m_maxBtn->setStyleSheet(R"(
                            QPushButton
                            {
                            border: none;
                            border-image: url(":/image/magnify.png");
                            background-color: transparent;
                            padding: 0px;
                            margin-right: 2px;
                            }
                            QPushButton:hover
                            {
                            border-image: url(":/image/magnifyHover.png");
                            }
                            QPushButton:pressed
                            {
                            outline: none;
                            }
                            QPushButton:focus
                            {
                            outline: none;
                            }
                            /* 设置提示词 */
                            QToolTip
                            {
                            background-color: rgba(255,255,255,0.5);  /* 背景颜色 */
                            color: balck;             /* 文字颜色 */
                            border: none;  /* 边框 */
                            font-size: 12px;            /* 字体大小 */
                            padding: 3px;               /* 内边距 */
                            border-radius: 3px;         /* 圆角 */
                            height: 10px;
                            }
                            )");

    // 关闭按钮
    m_closeBtn = new QPushButton(this);
    m_closeBtn->setFixedSize(30, 30);
    m_closeBtn->setToolTip("关闭窗口");  // 鼠标悬停提示
    m_closeBtn->setStyleSheet(R"(
                              QPushButton
                              {
                              border: none;
                              border-image: url(":/image/close.png");
                              background-color: transparent;
                              padding: 0px;
                              margin-right: 2px;
                              }
                              QPushButton:hover
                              {
                              border-image: url(":/image/closeHover.png");
                              }
                              QPushButton:pressed
                              {
                              outline: none;
                              }
                              QPushButton:focus
                              {
                              outline: none;
                              }
                              /* 设置提示词 */
                              QToolTip
                              {
                              background-color: rgba(255,255,255,0.5);  /* 背景颜色 */
                              color: balck;             /* 文字颜色 */
                              border: none;  /* 边框 */
                              font-size: 12px;            /* 字体大小 */
                              padding: 3px;               /* 内边距 */
                              border-radius: 3px;         /* 圆角 */
                              height: 10px;
                              }
                              )");
    // 3. 创建布局并添加控件
    m_layout = new QHBoxLayout(this);
    m_layout->addWidget(m_titleLabel);       // 标题左对齐
    m_layout->addStretch();                  // 左侧拉伸，使Logo居中
    m_layout->addStretch();                  // 右侧拉伸，使Logo居中
    m_layout->addWidget(m_minBtn);           // 最小化按钮
    m_layout->addWidget(m_maxBtn);           // 最大化按钮
    m_layout->addWidget(m_closeBtn);         // 关闭按钮
    m_layout->setContentsMargins(8, 0, 0, 0); // 左内边距8px，匹配系统标题栏
    m_layout->setSpacing(5);                 // 按钮间距0
    // 设置布局到标题栏
    setLayout(m_layout);
    Logger::instance()->log(Logger::Debug, "CustomTitleBar", "标题栏控件初始化完成");
}

/**
 * @brief 初始化信号槽连接
 */
void CustomTitleBar::initConnections()
{
    Logger::instance()->log(Logger::Debug, "CustomTitleBar", "初始化信号槽连接");
    if (m_minBtn) {
        connect(m_minBtn, &QPushButton::clicked, this, [this]() {
            if (auto *mainWin = qobject_cast<QMainWindow*>(window())) {
                Logger::instance()->log(Logger::Debug, "CustomTitleBar", "最小化窗口");
                mainWin->showMinimized();
            }
        });
    }
    if (m_maxBtn)
    {
        connect(m_maxBtn, &QPushButton::clicked, this, [this]() {
            if (auto *mainWin = qobject_cast<QMainWindow*>(window())) {
                if (mainWin->isMaximized())
                {
                    Logger::instance()->log(Logger::Debug, "CustomTitleBar", "还原窗口");
                    mainWin->showNormal();
                    m_maxBtn->setFixedSize(30, 30);
                    m_maxBtn->setToolTip("放大窗口");  // 鼠标悬停提示
                    m_maxBtn->setStyleSheet(R"(
                                            QPushButton
                                            {
                                            border: none;
                                            border-image: url(":/image/magnify.png");
                                            background-color: transparent;
                                            padding: 0px;
                                            margin-right: 2px;
                                            }
                                            QPushButton:hover
                                            {
                                            border-image: url(":/image/magnifyHover.png");
                                            }
                                            QPushButton:pressed
                                            {
                                            outline: none;
                                            }
                                            QPushButton:focus
                                            {
                                            outline: none;
                                            }
                                            /* 设置提示词 */
                                            QToolTip
                                            {
                                            background-color: rgba(255,255,255,0.5);  /* 背景颜色 */
                                            color: balck;             /* 文字颜色 */
                                            border: none;  /* 边框 */
                                            font-size: 12px;            /* 字体大小 */
                                            padding: 3px;               /* 内边距 */
                                            border-radius: 3px;         /* 圆角 */
                                            height: 10px;
                                            }
                                            )");

                }
                else
                {
                    Logger::instance()->log(Logger::Debug, "CustomTitleBar", "最大化窗口");
                    mainWin->showMaximized();
                    m_maxBtn->setFixedSize(30, 30);
                    m_maxBtn->setToolTip("缩小窗口");  // 鼠标悬停提示
                    m_maxBtn->setStyleSheet(R"(
                                            QPushButton
                                            {
                                            border: none;
                                            border-image: url(":/image/reduce.png");
                                            background-color: transparent;
                                            padding: 0px;
                                            margin-right: 2px;
                                            }
                                            QPushButton:hover
                                            {
                                            border-image: url(":/image/reduceHover.png");
                                            }
                                            QPushButton:pressed
                                            {
                                            outline: none;
                                            }
                                            QPushButton:focus
                                            {
                                            outline: none;
                                            }
                                            /* 设置提示词 */
                                            QToolTip
                                            {
                                            background-color: rgba(255,255,255,0.5);  /* 背景颜色 */
                                            color: balck;             /* 文字颜色 */
                                            border: none;  /* 边框 */
                                            font-size: 12px;            /* 字体大小 */
                                            padding: 3px;               /* 内边距 */
                                            border-radius: 3px;         /* 圆角 */
                                            height: 10px;
                                            }
                                            )");
                }
            }
        });
    }
    // 关闭按钮连接保持不变
    connect(m_closeBtn, &QPushButton::clicked, this, [this]() {
        Logger::instance()->log(Logger::Debug, "CustomTitleBar", "关闭窗口");
        window()->close();
    });
    Logger::instance()->log(Logger::Debug, "CustomTitleBar", "信号槽连接初始化完成");
}

/**
 * @brief 重写鼠标按下事件，记录拖动偏移量
 * @param event 鼠标事件
 */
void CustomTitleBar::mousePressEvent(QMouseEvent *event)
{
    // 只有点击标题栏空白区域（非按钮）才触发拖动
    if (event->button() == Qt::LeftButton && !m_closeBtn->geometry().contains(event->pos())) {
        // 获取标题栏所属的顶级主窗口
        QMainWindow *mainWindow = qobject_cast<QMainWindow*>(this->window());
        if (mainWindow) {
            // 关键：偏移量 = 鼠标全局位置 - 主窗口左上角位置
            m_dragOffset = event->globalPos() - mainWindow->frameGeometry().topLeft();
            Logger::instance()->log(Logger::Debug, "CustomTitleBar", "开始拖动窗口");
        }
    }
    QWidget::mousePressEvent(event); // 保留父类事件处理
}

/**
 * @brief 重写鼠标移动事件，实现窗口拖动
 * @param event 鼠标事件
 */
void CustomTitleBar::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        QMainWindow *mainWindow = qobject_cast<QMainWindow*>(this->window());
        if (mainWindow) {
            // 计算主窗口新位置：当前鼠标全局位置 - 偏移量
            QPoint newPos = event->globalPos() - m_dragOffset;
            mainWindow->move(newPos); // 移动主窗口（核心！）
        }
    }
    QWidget::mouseMoveEvent(event); // 保留父类事件处理
}

