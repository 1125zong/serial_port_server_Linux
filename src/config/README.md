# NPort 配置管理模块

Qt5.12 C++ 实现的NPort设备配置管理系统。

## 快速开始

### 1. 集成到MainWindow

```cpp
// mainwindow.h
#include "src/config/views/ConfigDialog.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
private slots:
    void onOpenConfigDialog();
private:
    ConfigDialog* m_configDialog;
};

// mainwindow.cpp
void MainWindow::onOpenConfigDialog() {
    if (!m_configDialog) {
        m_configDialog = new ConfigDialog(this);
    }
    m_configDialog->show();
}
```

### 2. 编译

```bash
cd QT
qmake
make
```

### 3. 运行

需要root权限访问 `/etc/wq_nportd.cf`

## 功能特性

- ✅ 配置文件管理 (wq_nportd.cf)
- ✅ 批量添加端口 (1/4/8/16/32)
- ✅ 选择性添加端口 (任意组合)
- ✅ 删除设备/端口
- ✅ 冗余模式支持
- ✅ SSL加密支持
- ✅ 自动备份恢复
- ✅ 配置验证

## 架构

```
src/config/
├── models/       # 数据模型
├── utils/        # 工具类
├── controllers/  # 业务逻辑
└── views/        # 用户界面
```

## 文档

- `INTEGRATION_GUIDE.md` - 详细集成指南
- `IMPLEMENTATION_STATUS.md` - 实现状态
- `COMPLETION_SUMMARY.md` - 完成总结

## 许可

与主项目相同
