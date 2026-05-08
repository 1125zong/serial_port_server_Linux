# NPort 配置管理集成指南

## 概述

本文档说明如何将NPort配置管理模块集成到现有的Qt5.12主应用程序中。

## 已完成的组件

### 1. 数据模型层 (Models)
- `PortModel` - 单个端口配置
- `DeviceModel` - 设备及其端口管理
- `ConfigurationData` - 完整配置状态管理

### 2. 工具类 (Utils)
- `InputValidator` - IP、端口、Minor、TTY验证
- `PortCalculator` - 端口号计算
- `TtyGenerator` - TTY名称生成

### 3. 控制器层 (Controllers)
- `BackupManager` - 配置备份/恢复
- `CommandExecutor` - 执行系统命令（mxaddsvr, mxdelsvr等）
- `ConfigManager` - 高级配置操作

### 4. 视图层 (Views)
- `ConfigDialog` - 主配置对话框
- `BatchAddDialog` - 批量添加对话框
- `SelectiveAddDialog` - 选择性添加对话框

## 集成步骤

### 步骤1: 在MainWindow中添加菜单项

编辑 `QT/mainwindow.ui` 或在代码中添加菜单：

```cpp
// 在mainwindow.h中添加
#include "src/config/views/ConfigDialog.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onOpenConfigDialog();

private:
    Ui::MainWindow *ui;
    ConfigDialog* m_configDialog;
};
```

### 步骤2: 实现菜单动作

在 `mainwindow.cpp` 中：

```cpp
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "src/config/views/ConfigDialog.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_configDialog(nullptr)
{
    ui->setupUi(this);
    
    // 创建菜单项
    QMenu* configMenu = menuBar()->addMenu("配置管理(&C)");
    QAction* nportConfigAction = configMenu->addAction("NPort 配置(&N)");
    connect(nportConfigAction, &QAction::triggered, this, &MainWindow::onOpenConfigDialog);
}

MainWindow::~MainWindow()
{
    delete ui;
    if (m_configDialog) {
        delete m_configDialog;
    }
}

void MainWindow::onOpenConfigDialog()
{
    // 单例模式：只创建一个实例
    if (!m_configDialog) {
        m_configDialog = new ConfigDialog(this);
        connect(m_configDialog, &QDialog::finished, [this]() {
            // 对话框关闭后可以选择保留或删除
            // m_configDialog->deleteLater();
            // m_configDialog = nullptr;
        });
    }
    
    m_configDialog->show();
    m_configDialog->raise();
    m_configDialog->activateWindow();
}
```

### 步骤3: 更新 .pro 文件

确保 `chuankou.pro` 包含所有新文件（已完成）：

```qmake
# Models
SOURCES += \
    src/config/models/PortModel.cpp \
    src/config/models/DeviceModel.cpp \
    src/config/models/ConfigurationData.cpp

HEADERS += \
    src/config/models/PortModel.h \
    src/config/models/DeviceModel.h \
    src/config/models/ConfigurationData.h

# Utils
SOURCES += \
    src/config/utils/InputValidator.cpp \
    src/config/utils/PortCalculator.cpp \
    src/config/utils/TtyGenerator.cpp

HEADERS += \
    src/config/utils/InputValidator.h \
    src/config/utils/PortCalculator.h \
    src/config/utils/TtyGenerator.h

# Controllers
SOURCES += \
    src/config/controllers/BackupManager.cpp \
    src/config/controllers/CommandExecutor.cpp \
    src/config/controllers/ConfigManager.cpp

HEADERS += \
    src/config/controllers/BackupManager.h \
    src/config/controllers/CommandExecutor.h \
    src/config/controllers/ConfigManager.h

# Views
SOURCES += \
    src/config/views/ConfigDialog.cpp \
    src/config/views/BatchAddDialog.cpp \
    src/config/views/SelectiveAddDialog.cpp

HEADERS += \
    src/config/views/ConfigDialog.h \
    src/config/views/BatchAddDialog.h \
    src/config/views/SelectiveAddDialog.h
```

### 步骤4: 编译和测试

```bash
cd QT
qmake
make
```

## 功能说明

### 主配置对话框 (ConfigDialog)

**功能：**
- 显示所有设备和端口的树形视图
- 刷新配置
- 批量添加端口
- 选择性添加端口
- 删除设备
- 删除端口
- 验证配置

**使用：**
1. 点击"刷新"加载当前配置
2. 选择设备或端口进行操作
3. 使用工具栏按钮执行操作

### 批量添加对话框 (BatchAddDialog)

**功能：**
- 一次性添加1/4/8/16/32个端口
- 自动计算端口号
- 支持SSL和冗余模式
- 实时预览

**使用：**
1. 输入设备IP地址
2. 选择端口数量
3. 设置起始端口
4. 可选：启用SSL
5. 可选：启用冗余模式并输入备份IP
6. 查看预览并确认

### 选择性添加对话框 (SelectiveAddDialog)

**功能：**
- 选择任意端口组合（1-32）
- 支持SSL和冗余模式
- 显示选择数量

**使用：**
1. 输入设备IP地址
2. 勾选要添加的端口
3. 设置起始端口
4. 可选：启用SSL
5. 可选：启用冗余模式并输入备份IP
6. 确认添加

## 配置文件

**位置：** `/etc/wq_nportd.cf`

**格式：**
```
ttymajor=34
calloutmajor=39
#[Minor] [ServerIP]  [data]  [cmd]   [FIFO]  [SSL]   [ttyName] [coutName] [interface][mode][BackIP]
0       192.168.1.100   4001    4002    1       0       ttyr00  cur00   0       0
1       192.168.1.100   4003    4004    1       0       ttyr01  cur01   0       0
```

## 权限要求

配置管理需要root权限来执行以下操作：
- 修改 `/etc/wq_nportd.cf`
- 执行 `mxaddsvr`, `mxdelsvr`, `mxcfmat`, `mxrmnod`
- 重新加载守护进程

程序会自动使用 `pkexec` 或 `sudo` 进行权限提升。

## 备份机制

所有修改操作会自动创建备份：
- 备份文件格式：`/etc/wq_nportd.cf.backup.YYYYMMDD_HHMMSS`
- 操作失败时自动恢复
- 手动恢复：复制备份文件覆盖原文件

## 错误处理

所有操作都有完整的错误处理：
- 输入验证（IP地址、端口号、Minor号等）
- 文件I/O错误
- 命令执行错误
- 自动备份恢复

错误信息会通过对话框显示给用户。

## 调试

启用Qt调试输出查看详细日志：

```bash
QT_LOGGING_RULES="*.debug=true" ./chuankou
```

日志包括：
- 配置文件解析
- 设备和端口操作
- 命令执行
- 错误信息

## 注意事项

1. **配置文件路径**：默认为 `/etc/wq_nportd.cf`，可在 `ConfigDialog.cpp` 中修改
2. **权限**：需要root权限执行系统命令
3. **备份**：每次修改前自动备份，建议定期清理旧备份
4. **守护进程**：修改后会自动重新加载守护进程
5. **并发**：不支持多个实例同时修改配置

## 故障排除

### 问题：无法打开配置文件
**解决：** 检查文件路径和权限

### 问题：命令执行失败
**解决：** 
- 确保 `mxaddsvr`, `mxdelsvr` 等命令在PATH中
- 检查是否有root权限
- 查看错误信息

### 问题：守护进程重新加载失败
**解决：**
- 检查 `systemctl` 或 `service` 命令是否可用
- 手动重启守护进程：`sudo systemctl restart npreal2d`

## 扩展功能（可选）

### 添加日志系统

创建 `ConfigLogger` 类记录所有操作：

```cpp
class ConfigLogger {
public:
    static void log(const QString& message);
    static void error(const QString& message);
};
```

### 添加配置导入/导出

支持从其他文件导入配置或导出当前配置。

### 添加批量操作

支持一次性操作多个设备。

## 总结

NPort配置管理模块已完全实现，只需简单的MainWindow集成即可使用。所有核心功能都已完成并经过测试。
