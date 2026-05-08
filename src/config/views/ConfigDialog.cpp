#include "ConfigDialog.h"
#include "BatchAddDialog.h"
#include "SelectiveAddDialog.h"
#include <QMessageBox>
#include <QHeaderView>
#include <QIcon>
#include <QDebug>

const QString ConfigDialog::CONFIG_FILE_PATH = "/usr/lib/wq_nport/driver/wq_nportd.cf";

ConfigDialog::ConfigDialog(QWidget *parent)
    : QDialog(parent)
    , m_mainLayout(nullptr)
    , m_toolbar(nullptr)
    , m_treeWidget(nullptr)
    , m_statusBar(nullptr)
    , m_configManager(nullptr)
{
    m_configManager = new ConfigManager();
    
    setupUi();
    
    // 加载配置
    onRefreshClicked();
}

ConfigDialog::~ConfigDialog()
{
    delete m_configManager;
}

void ConfigDialog::setupUi()
{
    setWindowTitle("NPort 配置管理");
    resize(1000, 600);
    
    // 创建主布局
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);
    
    // 创建组件
    createToolbar();
    createTreeWidget();
    createStatusBar();
    
    // 添加到布局
    m_mainLayout->addWidget(m_toolbar);
    m_mainLayout->addWidget(m_treeWidget);
    m_mainLayout->addWidget(m_statusBar);
    
    setLayout(m_mainLayout);
}

void ConfigDialog::createToolbar()
{
    m_toolbar = new QToolBar(this);
    m_toolbar->setMovable(false);
    m_toolbar->setIconSize(QSize(24, 24));
    
    // 刷新操作
    m_refreshAction = m_toolbar->addAction("刷新");
    m_refreshAction->setToolTip("重新加载配置文件");
    connect(m_refreshAction, &QAction::triggered, this, &ConfigDialog::onRefreshClicked);
    
    m_toolbar->addSeparator();
    
    // 批量添加操作
    m_batchAddAction = m_toolbar->addAction("批量添加");
    m_batchAddAction->setToolTip("批量添加端口 (1/4/8/16)");
    connect(m_batchAddAction, &QAction::triggered, this, &ConfigDialog::onBatchAddClicked);
    
    // 选择性添加操作
    m_selectiveAddAction = m_toolbar->addAction("选择性添加");
    m_selectiveAddAction->setToolTip("选择性添加端口 (任意组合)");
    connect(m_selectiveAddAction, &QAction::triggered, this, &ConfigDialog::onSelectiveAddClicked);
    
    m_toolbar->addSeparator();
    
    // 删除设备操作
    m_deleteDeviceAction = m_toolbar->addAction("删除设备");
    m_deleteDeviceAction->setToolTip("删除选中的设备及其所有端口");
    connect(m_deleteDeviceAction, &QAction::triggered, this, &ConfigDialog::onDeleteDeviceClicked);
    
    // 删除端口操作
    m_deletePortsAction = m_toolbar->addAction("删除端口");
    m_deletePortsAction->setToolTip("删除选中的端口");
    connect(m_deletePortsAction, &QAction::triggered, this, &ConfigDialog::onDeletePortsClicked);
    
    m_toolbar->addSeparator();
    
    // 验证配置操作
    m_validateAction = m_toolbar->addAction("验证配置");
    m_validateAction->setToolTip("验证当前配置的正确性");
    connect(m_validateAction, &QAction::triggered, this, &ConfigDialog::onValidateClicked);
}

void ConfigDialog::createTreeWidget()
{
    m_treeWidget = new QTreeWidget(this);
    m_treeWidget->setColumnCount(COL_COUNT);
    
    // 设置标题
    QStringList headers;
    headers << "名称" << "IP地址" << "数据端口" << "命令端口"
            << "Minor" << "TTY名称" << "SSL" << "模式" << "备份IP";
    m_treeWidget->setHeaderLabels(headers);
    
    // 配置树
    m_treeWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_treeWidget->setAlternatingRowColors(true);
    m_treeWidget->setSortingEnabled(false);
    
    // 调整列宽
    m_treeWidget->header()->setStretchLastSection(true);
    for (int i = 0; i < COL_COUNT - 1; ++i) {
        m_treeWidget->header()->setSectionResizeMode(i, QHeaderView::ResizeToContents);
    }
    
    // 连接信号
    connect(m_treeWidget, &QTreeWidget::itemSelectionChanged,
            this, &ConfigDialog::onTreeItemSelectionChanged);
}

void ConfigDialog::createStatusBar()
{
    m_statusBar = new QStatusBar(this);
    m_statusBar->showMessage("就绪");
}

void ConfigDialog::populateTree()
{
    clearTree();
    
    const ConfigurationData& config = m_configManager->getConfiguration();
    QList<ConfigDeviceModel> devices = config.getDevices();
    
    // 按IP排序设备
    std::sort(devices.begin(), devices.end(),
              [](const ConfigDeviceModel& a, const ConfigDeviceModel& b) {
        return a.ip() < b.ip();
    });
    
    // 向树中添加设备和端口
    for (const ConfigDeviceModel& device : devices) {
        // 创建设备项
        QTreeWidgetItem* deviceItem = new QTreeWidgetItem(m_treeWidget);
        deviceItem->setText(COL_NAME, QString("设备: %1").arg(device.ip()));
        deviceItem->setText(COL_IP, device.ip());
        deviceItem->setData(COL_NAME, Qt::UserRole, "device");
        
        // 使设备项加粗
        QFont font = deviceItem->font(COL_NAME);
        font.setBold(true);
        deviceItem->setFont(COL_NAME, font);
        
        // 添加端口
        QList<PortModel> ports = device.ports();
        std::sort(ports.begin(), ports.end(),
                  [](const PortModel& a, const PortModel& b) {
            return a.minor() < b.minor();
        });
        
        for (const PortModel& port : ports) {
            QTreeWidgetItem* portItem = new QTreeWidgetItem(deviceItem);
            portItem->setText(COL_NAME, QString("端口 %1").arg(port.minor()));
            portItem->setText(COL_IP, port.ip());
            portItem->setText(COL_DATA_PORT, QString::number(port.dataPort()));
            portItem->setText(COL_CMD_PORT, QString::number(port.cmdPort()));
            portItem->setText(COL_MINOR, QString::number(port.minor()));
            portItem->setText(COL_TTY, port.ttyName());
            portItem->setText(COL_SSL, port.ssl() ? "是" : "否");
            portItem->setText(COL_MODE, port.mode() == 1 ? "冗余" : "普通");
            portItem->setText(COL_BACKUP_IP, port.backupIp());
            portItem->setData(COL_NAME, Qt::UserRole, "port");
            portItem->setData(COL_MINOR, Qt::UserRole, port.minor());
        }
        
        deviceItem->setExpanded(true);
    }
    
    // 更新状态
    int deviceCount = devices.size();
    int portCount = config.totalPortCount();
    m_statusBar->showMessage(QString("设备: %1, 端口: %2").arg(deviceCount).arg(portCount));
    
    updateActionStates();
}

void ConfigDialog::clearTree()
{
    m_treeWidget->clear();
}

void ConfigDialog::updateActionStates()
{
    QList<QTreeWidgetItem*> selectedItems = m_treeWidget->selectedItems();
    
    bool hasSelection = !selectedItems.isEmpty();
    bool hasDeviceSelection = false;
    bool hasPortSelection = false;
    
    for (QTreeWidgetItem* item : selectedItems) {
        QString type = item->data(COL_NAME, Qt::UserRole).toString();
        if (type == "device") {
            hasDeviceSelection = true;
        } else if (type == "port") {
            hasPortSelection = true;
        }
    }
    
    m_deleteDeviceAction->setEnabled(hasDeviceSelection);
    m_deletePortsAction->setEnabled(hasPortSelection);
}

QString ConfigDialog::getSelectedDeviceIp() const
{
    QList<QTreeWidgetItem*> selectedItems = m_treeWidget->selectedItems();
    if (selectedItems.isEmpty()) {
        return QString();
    }
    
    QTreeWidgetItem* item = selectedItems.first();
    QString type = item->data(COL_NAME, Qt::UserRole).toString();
    
    if (type == "device") {
        return item->text(COL_IP);
    } else if (type == "port") {
        // 获取父设备IP
        QTreeWidgetItem* parent = item->parent();
        if (parent) {
            return parent->text(COL_IP);
        }
    }
    
    return QString();
}

QList<int> ConfigDialog::getSelectedPortMinors() const
{
    QList<int> minors;
    QList<QTreeWidgetItem*> selectedItems = m_treeWidget->selectedItems();
    
    for (QTreeWidgetItem* item : selectedItems) {
        QString type = item->data(COL_NAME, Qt::UserRole).toString();
        if (type == "port") {
            int minor = item->data(COL_MINOR, Qt::UserRole).toInt();
            minors.append(minor);
        }
    }
    
    return minors;
}

void ConfigDialog::showError(const QString& title, const QString& message)
{
    QMessageBox::critical(this, title, message);
}

void ConfigDialog::showInfo(const QString& title, const QString& message)
{
    QMessageBox::information(this, title, message);
}

// Slots implementation

void ConfigDialog::onRefreshClicked()
{
    m_statusBar->showMessage("正在加载配置...");
    
    ConfigManager::OperationResult result = m_configManager->loadConfiguration(CONFIG_FILE_PATH);
    
    if (!result.first) {
        showError("加载失败", QString("无法加载配置文件:\n%1").arg(result.second));
        m_statusBar->showMessage("加载失败");
        return;
    }
    
    populateTree();
    m_statusBar->showMessage("配置已刷新");
}

void ConfigDialog::onBatchAddClicked()
{
    BatchAddDialog dialog(this);
    
    if (dialog.exec() == QDialog::Accepted) {
        // 从对话框获取参数
        QString ip = dialog.getIpAddress();
        int portCount = dialog.getPortCount();
        int startPort = dialog.getStartPort();
        bool ssl = dialog.getSslEnabled();
        bool redundant = dialog.getRedundantMode();
        QString backupIp = dialog.getBackupIp();
        
        m_statusBar->showMessage("正在添加端口...");
        
        // 执行批量添加
        ConfigManager::OperationResult result = m_configManager->batchAddPorts(
                    ip, portCount, startPort, ssl, redundant, backupIp
                    );
        
        if (!result.first)
        {
            showError("添加失败", QString("批量添加端口失败:\n%1").arg(result.second));
            m_statusBar->showMessage("添加失败");
            return;
        }
        
        // 刷新树
        populateTree();
        showInfo("添加成功", result.second);
        m_statusBar->showMessage("端口添加成功");
    }
}

void ConfigDialog::onSelectiveAddClicked()
{
    SelectiveAddDialog dialog(this);
    
    if (dialog.exec() == QDialog::Accepted) {
        // 从对话框获取参数
        QString ip = dialog.getIpAddress();     //获取输入的IP地址
        QList<int> portIndices = dialog.getSelectedPortIndices();   //获取选中的端口下标
        int startPort = dialog.getStartPort();      //获取起始端口
        bool ssl = dialog.getSslEnabled();      //是否启用SSL加密
        bool redundant = dialog.getRedundantMode();     //是否启用冗余模式
        QString backupIp = dialog.getBackupIp();    //获取备份IP地址
        
        m_statusBar->showMessage("正在添加端口...");
        
        // 执行选择性添加
        ConfigManager::OperationResult result = m_configManager->selectiveAddPorts(
                    ip, portIndices, startPort, ssl, redundant, backupIp
                    );
        
        if (!result.first) {
            showError("添加失败", QString("选择性添加端口失败:\n%1").arg(result.second));
            m_statusBar->showMessage("添加失败");
            return;
        }
        
        // 刷新树
        populateTree();
        showInfo("添加成功", result.second);
        m_statusBar->showMessage("端口添加成功");
    }
}

void ConfigDialog::onDeleteDeviceClicked()
{
    QString deviceIp = getSelectedDeviceIp();
    if (deviceIp.isEmpty()) {
        showError("删除失败", "请先选择要删除的设备");
        return;
    }
    
    // 确认删除
    QMessageBox::StandardButton reply = QMessageBox::question(
                this,
                "确认删除",
                QString("确定要删除设备 %1 及其所有端口吗？").arg(deviceIp),
                QMessageBox::Yes | QMessageBox::No
                );
    
    if (reply != QMessageBox::Yes) {
        return;
    }
    
    m_statusBar->showMessage("正在删除设备...");
    
    // Execute delete
    ConfigManager::OperationResult result = m_configManager->deleteDevice(deviceIp);
    
    if (!result.first) {
        showError("删除失败", QString("删除设备失败:\n%1").arg(result.second));
        m_statusBar->showMessage("删除失败");
        return;
    }
    
    // Refresh tree
    populateTree();
    showInfo("删除成功", result.second);
    m_statusBar->showMessage("设备删除成功");
}

void ConfigDialog::onDeletePortsClicked()
{
    QList<int> minors = getSelectedPortMinors();
    if (minors.isEmpty()) {
        showError("删除失败", "请先选择要删除的端口");
        return;
    }
    
    QString deviceIp = getSelectedDeviceIp();
    if (deviceIp.isEmpty()) {
        showError("删除失败", "无法确定设备IP");
        return;
    }
    
    // 确认删除
    QMessageBox::StandardButton reply = QMessageBox::question(
                this,
                "确认删除",
                QString("确定要删除 %1 个端口吗？").arg(minors.size()),
                QMessageBox::Yes | QMessageBox::No
                );
    
    if (reply != QMessageBox::Yes) {
        return;
    }
    
    m_statusBar->showMessage("正在删除端口...");
    
    // Execute delete
    ConfigManager::OperationResult result = m_configManager->deleteSelectedPorts(deviceIp, minors);
    
    if (!result.first) {
        showError("删除失败", QString("删除端口失败:\n%1").arg(result.second));
        m_statusBar->showMessage("删除失败");
        return;
    }
    
    // Refresh tree
    populateTree();
    showInfo("删除成功", result.second);
    m_statusBar->showMessage("端口删除成功");
}

void ConfigDialog::onValidateClicked()
{
    m_statusBar->showMessage("正在验证配置...");
    
    ConfigManager::OperationResult result = m_configManager->validateConfiguration();
    
    if (!result.first) {
        showError("验证失败", QString("配置验证失败:\n%1").arg(result.second));
        m_statusBar->showMessage("验证失败");
        return;
    }
    
    showInfo("验证成功", result.second);
    m_statusBar->showMessage("配置验证通过");
}

void ConfigDialog::onTreeItemSelectionChanged()
{
    updateActionStates();
}
