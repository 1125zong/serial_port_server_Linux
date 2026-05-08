#include "mainwindow.h"
#include "build-chuankou/ui_mainwindow.h"
#include "src/model/NetworkConfig.h"
#include "src/communication/TcpClient.h"
#include "src/model/SerialPortConfig.h"
#include <QFileDialog>
#include <QStyleFactory>
#include "src/config/views/ConfigDialog.h"  //2026.01.20新增
#include "src/utils/Logger.h"  // 日志系统
#include <QMetaObject>
#include <QTime>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_configDialog(nullptr)
{
    ui->setupUi(this);

    m_statusLabel = new QLabel("欢迎使用", this);
    m_statusLabel->setStyleSheet(
                "QLabel {"
                " color: white;"
                " font-size: 14px;"
                " height: 24px;"
                " padding-left: 10px;"
                "}"
                );
    ui->statusbar->addWidget(m_statusLabel, 0);

    QList<QComboBox *> AllComBox = findChildren<QComboBox *>();
    for (QComboBox *combo : AllComBox)
    {
        combo->setView(new QListView());
    }

    initTables();               // 一键初始化所有表格
    initTitleBar();             //该函数完成了主窗口的「界面样式定制 + 布局搭建」
    ui->stackedWidget->setCurrentIndex(0);

    //使用单例模式获取UdpDiscovery实例
    m_udpDiscovery = UdpDiscovery::instance(this);
    /* 创建 Controller 并挂接信号 */
    m_devCtl = new DeviceController(this);

    // 创建正则表达式验证器
    QRegularExpression regExp("^[A-Za-z0-9]*$");  // 只允许字母和数字
    QRegularExpressionValidator *validator = new QRegularExpressionValidator(regExp, this);

    connect(m_devCtl, &DeviceController::deviceDiscovered,  this, &MainWindow::onDeviceDiscovered);
    connect(m_devCtl, &DeviceController::searchCompleted,   this, &MainWindow::onSearchCompleted);
    connect(m_devCtl, &DeviceController::udpBindSucceeded,  this, [this](quint16 port) {
        appendDeviceLog(QString("绑定 UDP 端口成功：%1").arg(port));
    });
    connect(m_devCtl, &DeviceController::connected,         this, &MainWindow::onDeviceConnected);
    connect(m_devCtl, &DeviceController::disconnected,      this, &MainWindow::onDeviceDisconnected);
    connect(m_devCtl, &DeviceController::connectionError,   this, &MainWindow::onConnectionError);
    connect(m_devCtl, &DeviceController::operationSuccess,  this, &MainWindow::onOperationSuccess);
    connect(m_devCtl, &DeviceController::operationFailed,   this, &MainWindow::onOperationFailed);

    /*--------------------------已优化------------------------*/
    connect(m_devCtl, &DeviceController::setWordToUnlockLab, this, [this](const QString &Word)
    {
        QMessageBox::information(this, "操作", "设备已成功锁定！");
        ui->lineEdit->setEnabled(true);
        ui->unlockPassWord_lab->setText(QString("锁定密码为：%1").arg(Word));
        ui->unlockPassWord_lab->setTextInteractionFlags(Qt::TextSelectableByMouse);
        appendDeviceLog(QString("锁定设备成功：%1，解锁密码=%2").arg(currentCheckedDeviceText(), Word));
    });
    connect(m_devCtl, &DeviceController::operateError, this, [this](const QString &action){
        QMessageBox::information(this, "操作", action);
        if (action == "端口锁定失败")
        {
            appendDeviceLog(QString("操作失败：锁定端口失败，端口：%1").arg(m_pendingLockPortNames.join("，")));
            m_pendingLockPortNames.clear();
        }
        else if (action == "端口解锁失败")
        {
            appendDeviceLog(QString("操作失败：解锁端口失败，端口：%1").arg(m_pendingUnlockPortNames.join("，")));
            m_pendingUnlockPortNames.clear();
        }
        else
        {
            appendDeviceLog(QString("操作失败：%1").arg(action));
        }
    });
    connect(m_devCtl, &DeviceController::operateOK, this, [this](const QString &action){
        QMessageBox::information(this, "操作", action);
        if (action == "端口锁定成功")
        {
            appendDeviceLog(QString("锁定成功：%1").arg(m_pendingLockPortNames.join("，")));
            m_pendingLockPortNames.clear();
        }
        else if (action == "端口解锁成功")
        {
            appendDeviceLog(QString("解锁成功：%1").arg(m_pendingUnlockPortNames.join("，")));
            m_pendingUnlockPortNames.clear();
        }
        else if (action == "设备解锁成功")
        {
            appendDeviceLog(QString("解锁设备成功：%1").arg(currentCheckedDeviceText()));
        }
        else if (action == "设备名修改成功")
        {
            appendDeviceLog(QString("修改设备名称成功：%1").arg(currentCheckedDeviceText()));
        }
        else if (action != "配置成功")
        {
            appendDeviceLog(QString("操作成功：%1").arg(action));
        }
        if (action == "设备解锁成功")
        {
            ui->lineEdit->clear();
            ui->unlockPassWord_lab->setText("锁定密码显示");
        }
        if (action == "设备名修改成功")
        {
            ui->new_name->clear();
        }
    });
    /*---------------------------------------------------------------*/

    connect(m_devCtl, &DeviceController::upgradeProgress,this, &MainWindow::onUpgradeProgress);
    connect(m_devCtl, &DeviceController::updateOK,this, &MainWindow::onUpgradeSuccess);     // 固件升级成功，进行收尾
    connect(m_devCtl, &DeviceController::upgradeSuccess, this, &MainWindow::upgradeFinish);
    connect(m_devCtl, &DeviceController::upgradeFailed, this, &MainWindow::onUpgradeFailed);
    connect(m_devCtl, &DeviceController::showUpgradeMask,this, &MainWindow::showUpgradeMask);
    connect(ui->tableWidgetSearch, &QTableWidget::itemChanged, this, &MainWindow::onItemChanged);
    //connect(m_devCtl, &DeviceController::upgradeHandshakeRequested,
    //        this,   &MainWindow::startUpgradeFlow);
    ui->tabWidget->setTabText(0, "参数设置");
    ui->tabWidget->setTabText(1, "模式设置");
    ui->tabWidget->setTabText(2, "端口模式查询");

    ui->new_name->setValidator(validator);
    ui->new_name->setPlaceholderText("只能输入字母和数字，且不超过20字符");
    ui->alias_lineEdit->setValidator(validator);
    ui->alias_lineEdit->setPlaceholderText("只能输入字母和数字，且不超过19字符");
    ui->new_name->setMaxLength(20);
    ui->alias_lineEdit->setMaxLength(19);
    
    bindDataToUI();

    
    // 创建菜单项---2026.01.20新增
    //    QMenu* configMenu = menuBar()->addMenu("配置管理(&C)");
    //    QAction* nportConfigAction = configMenu->addAction("NPort 配置(&N)");
    //    connect(nportConfigAction, &QAction::triggered, this, &MainWindow::onOpenConfigDialog);
}
MainWindow::~MainWindow()
{
    /* 2. 断开所有可能还在发射的自定义信号 */
    if (m_devCtl)
    {
        m_devCtl->disconnect();   // 阻断所有信号
        m_devCtl->deleteLater();  // 异步安全销毁
    }

    /* 3. 清空可能还在排队的 lambda 连接 */
    disconnect(this, nullptr, nullptr, nullptr); // 彻底断开 this 的所有连接

    delete ui;

    //2026.01.20新增
    if (m_configDialog)
    {
        delete m_configDialog;
    }
}

void MainWindow::onPortLockStatusReady(const QList<PortLockInfo> &list)
{

    ui->tableWidget_4->setRowCount(list.size());
    for (int i = 0; i < list.size(); ++i)
    {
        const auto &v = list.at(i);
        ui->tableWidget_4->setItem(i, 1, new QTableWidgetItem(QString::number(v.portNum)));
        ui->tableWidget_4->setItem(i, 2, new QTableWidgetItem(v.lockState ? "已锁定" : "未锁定"));
        ui->tableWidget_4->setItem(i, 3, new QTableWidgetItem(v.lockMac));
    }
}

void MainWindow::startPackageUpgrade(const QString &filePath)
{
    // 1. 读取并验证 .pkg
    QByteArray pkg;     //.pkg文件的内容
    if (!validateFirmwarePackage(filePath, pkg))
    {
        QMessageBox::critical(this, "错误", "固件包验证失败！");
        return;
    }

    // 3. 发送整包
    m_devCtl->DeviceControllerStartPackageUpgrade(pkg);
}

//void MainWindow::startPackageUpgrade(const QByteArray &pkg)
//{
//    m_devCtl->startPackageUpgrade(pkg);   // 把数据传给控制器
//}


//void MainWindow::upgradeFinish()
//{
//    if (m_mask) {
//           m_mask->hide();
//           delete m_progress;
//           m_progress = nullptr;
//       }
//       for (auto btn : findChildren<QPushButton *>()) btn->setEnabled(true);
//}

/* ============================================================
一、设备搜索
============================================================ */
void MainWindow::on_Search_Btn_clicked()
{
    appendDeviceLog("点击搜索设备");

    if (TcpClient::TcpSocketState == true)
    {
        QMessageBox::information(this, "提示", "请先断开设备连接在进行设备搜索");
        appendDeviceLog("操作失败：设备已连接，请先断开设备后再搜索");
        return;
    }
    Logger::instance()->log(Logger::Info, "MainWindow", "开始搜索设备");
    appendDeviceLog("开始搜索设备");
    this->m_buttonCoolDownTimer = new QTimer(this);
    m_buttonCoolDownTimer->setSingleShot(true); // 单次定时器
    m_buttonCoolDownTimer->start(5000);

    // 将焦点设置到表格上，防止禁用按钮后焦点转移到清空列表按钮
    ui->tableWidgetSearch->setFocus();
    ui->Search_Btn->setEnabled(false);

    // 清除所有行--即清空表格tableWidgetSearch
    ui->tableWidgetSearch->setRowCount(0);
    m_statusLabel->setText("正在搜索设备...");
    m_devCtl->searchDevices();
    connect(m_buttonCoolDownTimer, &QTimer::timeout, this, [this]() {
        ui->Search_Btn->setEnabled(true);
        Logger::instance()->log(Logger::Debug, "MainWindow", "搜索按钮冷却结束，已重新启用");
    });
}

void MainWindow::onDeviceDiscovered(const DeviceInfo &dev)
{
    QString newMac = dev.lan1Mac.trimmed().toUpper();

    if (!newMac.isEmpty())
    {
        for (int i = 0; i < ui->tableWidgetSearch->rowCount(); ++i)
        {
            QTableWidgetItem *macItem = ui->tableWidgetSearch->item(i, 8);
            if (macItem && macItem->text().trimmed().toUpper() == newMac)
            {
                return; // MAC 已存在，不重复添加
            }
        }
    }

    int row = ui->tableWidgetSearch->rowCount();
    ui->tableWidgetSearch->insertRow(row);

    QTableWidgetItem *checkItem = new QTableWidgetItem();
    checkItem->setFlags(Qt::ItemIsUserCheckable |
                        Qt::ItemIsEnabled |
                        Qt::ItemIsSelectable);
    checkItem->setCheckState(Qt::Unchecked);
    ui->tableWidgetSearch->setItem(row, 0, checkItem);

    QString lockState = (dev.devLockState == QPORT_DeviceLockState::Locked) ? "已锁定" : "未锁定";
    ui->tableWidgetSearch->setItem(row, 1, new QTableWidgetItem(lockState));
    ui->tableWidgetSearch->setItem(row, 2, new QTableWidgetItem(dev.deviceModel));
    ui->tableWidgetSearch->setItem(row, 3, new QTableWidgetItem(dev.serialNum));
    ui->tableWidgetSearch->setItem(row, 4, new QTableWidgetItem(dev.deviceName));
    ui->tableWidgetSearch->setItem(row, 5, new QTableWidgetItem(dev.firmwareVer));

    QString state = deviceStateText(static_cast<quint8>(dev.deviceState));
    ui->tableWidgetSearch->setItem(row, 6, new QTableWidgetItem(state));
    ui->tableWidgetSearch->setItem(row, 7, new QTableWidgetItem(dev.lan1Ip));
    ui->tableWidgetSearch->setItem(row, 8, new QTableWidgetItem(dev.lan1Mac));
}


void MainWindow::onSearchCompleted(int count)
{
    Q_UNUSED(count);
    int deviceCount = ui->tableWidgetSearch->rowCount();

    m_statusLabel->setText(QString("共找到 %1 台设备").arg(deviceCount));
    Logger::instance()->log(Logger::Info, "MainWindow", QString("设备搜索完成，共找到 %1 台设备").arg(deviceCount));

    appendDeviceLog(QString("搜索完成：共发现 %1 台设备").arg(deviceCount));
    for (int row = 0; row < deviceCount; ++row)
    {
        appendDeviceLog(searchTableDeviceText(row));
    }
}


/* ============================================================
二、连接 / 断开
============================================================ */
void MainWindow::on_llogin_btn_clicked()
{
    if (TcpClient::TcpSocketState == false)      //未连接
    {
        // 返回选择行的索引
        int checkedRows = m_devCtl->getCheckedRows(ui->tableWidgetSearch, 0);
        // 检查复选框是否选中
        QTableWidgetItem *checkItem = checkedRows >= 0 ? ui->tableWidgetSearch->item(checkedRows, 0) : nullptr;
        if (checkedRows < 0 || (checkItem && checkItem->checkState() != Qt::Checked))
        {
            QMessageBox::warning(this, "提示", "请先选中要连接的设备");
            appendDeviceLog("操作失败：请先选中要连接的设备");
            return;
        }
        QString ip = ui->tableWidgetSearch->item(checkedRows, 7)->text();
        Logger::instance()->log(Logger::Info, "MainWindow", QString("正在连接设备，IP: %1").arg(ip));
        appendDeviceLog(QString("连接设备：%1").arg(searchTableDeviceText(checkedRows)));
        m_devCtl->connectToDevice(ip, 4000);
    }
    else
    {
        Logger::instance()->log(Logger::Debug, "MainWindow", "设备已连接，切换到主界面");
        ui->stackedWidget->setCurrentIndex(1);
    }
}

void MainWindow::onDeviceConnected()
{
    QMessageBox::information(this, "设备连接", QString("成功与设备：%1建立连接").arg(UdpDiscovery::dev.lan1Ip));
    m_statusLabel->setText(QString("已与设备 %1 建立连接").arg(UdpDiscovery::dev.lan1Ip));
    // 将复选框设置为不可交互状态
    int row = ui->tableWidgetSearch->rowCount();
    for (int i = 0; i < row; i++)
    {
        QTableWidgetItem *item =ui->tableWidgetSearch->item(i, 0);
        if (item)
        {
            Qt::ItemFlags flags = item->flags();
            flags &= ~Qt::ItemIsSelectable;
            flags &= ~Qt::ItemIsEnabled;

            item->setFlags(flags);
        }
    }


    ui->llogin_btn->setText("返回");
    TcpClient::TcpSocketState = true;
    ui->stackedWidget->setCurrentIndex(1);

    Logger::instance()->log(Logger::Info, "MainWindow", "设备连接成功");
    appendDeviceLog(QString("连接成功：%1").arg(currentCheckedDeviceText()));
}

void MainWindow::onDeviceDisconnected()
{
    // 先弹个小提示
    QMessageBox::information(this, "连接断开", "设备连接已断开，返回主页面。");
    m_statusLabel->setText(QString("已与设备 %1 断开连接").arg(UdpDiscovery::dev.lan1Ip));
    // 回到主页面（索引 0）
    ui->stackedWidget->setCurrentIndex(0);
    TcpClient::TcpSocketState = false;
    ui->llogin_btn->setText("登陆后台");

    int row = m_devCtl->getCheckedRows(ui->tableWidgetSearch, 0);
    QTableWidgetItem *checkItem = row >= 0 ? ui->tableWidgetSearch->item(row, 0) : nullptr;
    if (checkItem)
    {
        checkItem->setCheckState(Qt::Unchecked);
        Qt::ItemFlags flags = checkItem->flags();
        flags |= Qt::ItemIsSelectable;
        flags |= Qt::ItemIsEnabled;
        checkItem->setFlags(flags);
    }

    Logger::instance()->log(Logger::Info, "MainWindow", "设备连接断开");
    appendDeviceLog(QString("断开设备：%1").arg(currentCheckedDeviceText()));
}

void MainWindow::onConnectionError(const QString &err)
{
    QMessageBox::warning(this, "连接失败", err);
    Logger::instance()->log(Logger::Error, "MainWindow", QString("设备连接失败: %1").arg(err));
    appendDeviceLog(QString("操作失败：连接设备失败，原因：%1").arg(err));
}

/* ============================================================
三、读操作
============================================================ */
void MainWindow::on_overview_btn_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
    ui->stackedWidget_2->setCurrentIndex(0);
    appendDeviceLog(QString("读取配置：设备总览，%1").arg(currentCheckedDeviceText()));
    m_devCtl->readOverview();
}

void MainWindow::on_basic_settings_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
    ui->stackedWidget_2->setCurrentIndex(1);
    appendDeviceLog(QString("读取配置：网络配置，%1").arg(currentCheckedDeviceText()));
    m_devCtl->readNetworkConfig();
}

//设备连接以后，点击登录后台，点击设备总览按钮显示的界面。
void MainWindow::onOperationSuccess(const QString &op)
{
    if (op == "Read Overview")
    {
        Logger::instance()->log(Logger::Info, "MainWindow", "Read Overview");
        appendDeviceLog("读取配置成功：设备总览");
        auto m = m_devCtl->currentDevice();
        ui->textBrowser->clear();
        ui->textBrowser->setText(
                    QString(
                        "设备型号：%1\r\n"
                        "序列号：%2\r\n"
                        "设备名称：%3\r\n"
                        "固件版本：%4\r\n"
                        "设备状态：%5\r\n"
                        "网口数量：%6\r\n"
                        "IP1 地址：%7\r\n"
                        "MAC1 地址：%8\r\n"
                        "电源1：%12\r\n"
                        "LCD 显示：%11\r\n"
                        "设备锁定：%14"
                        )
                    .arg(m->deviceModel())
                    .arg(m->serialNumber())
                    .arg(m->deviceName())
                    .arg(m->firmwareVersion())
                    .arg(getDeviceStatusName(m->deviceState()))      // 枚举→中文
                    .arg(m->validLanCount())
                    .arg(m->lan1Ip())
                    .arg(m->lan1Mac())
                    .arg(m->lcdState() == QPORT_LCDState::HasLCD ? "有" : "无")
                    .arg(m->power1State() == QPORT_PowerState::PowerOn ? "On" : "Off")
                    .arg(m->lockState() == QPORT_DeviceLockState::Locked ? "已锁定" : "未锁定")
                    );


        auto ports = m->ports();
        ui->tableWidget->setRowCount(ports.size());
        ui->tableWidget->setColumnCount(6);          // 必须 6 列
        ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);  //设置禁止修改单元格内容
        ui->tableWidget->horizontalHeader()->setStretchLastSection(true);     //让最后一列的单元格自动拉伸，填满表格的剩余宽度
        ui->tableWidget->setHorizontalHeaderLabels({"端口","物理参数","工作模式","工作状态","锁定状态","接口类型"});

        for (int i = 0; i < ports.size(); ++i)
        {
            QList<QString>items;
            const PortStatus &p = ports.at(i);

            QString parityStr = [](PortParity p)
            {
                switch (p)
                {
                case PortParity::None:  return "n";
                case PortParity::Odd:   return "o";
                case PortParity::Even:  return "e";
                default:                return "?";
                }
            }(p.parity);
            QString modeStr = [](PortWorkMode m)
            {
                switch (m)
                {
                case PortWorkMode::TcpServer: return "TCP Server";
                case PortWorkMode::RealCom:   return "RealCOM";
                default:                      return "?";
                }
            }(p.workMode);

            // 先全部转成人类可读字符串
            QString connStr = (p.workStatus == PortWorkStatus::Connected) ? "已连接" : "未连接";
            QString lockStr = (p.lockStatus == PortLockStatus::Locked)    ? "已锁定" : "未锁定";
            Logger::instance()->log(Logger::Debug, "MainWindow", QString("Port %1 lockStatus: %2, lockStr: %3").arg(i+1).arg(static_cast<int>(p.lockStatus)).arg(lockStr));
            QString ifStr   = [](SerialPortInterfaceType t)
            {
                switch (t)
                {
                case SerialPortInterfaceType::RS232: return "RS-232";
                case SerialPortInterfaceType::RS422: return "RS-422";
                case SerialPortInterfaceType::RS485: return "RS-485-2";
                default:                         return "?";
                }
            }(p.ifType);
            items.append(QString::number(i+1));
            items.append(QString("%1,%2,%3,%4")
                         .arg(p.baudRate)
                         .arg(p.dataBits)
                         .arg(parityStr)
                         .arg(p.stopBits));
            items.append(modeStr);
            items.append(connStr);
            items.append(lockStr);
            items.append(ifStr);
            for(int j = 0; j < 6; j++)
            {
                // 再填表
                QTableWidgetItem* item = new QTableWidgetItem(items[j]);
                item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
                ui->tableWidget->setItem(i, j, item);
            }
        }

        Logger::instance()->log(Logger::Debug, "MainWindow", "[Table] loop finished");
    }
    if (op == "Read Network Config")
    {
        auto cfg = m_devCtl->currentDevice()->networkConfig();
        m_lastNetworkConfig = cfg;
        m_hasLastNetworkConfig = true;
        /* ========== LAN1 ========== */
        ui->lan1_ip      ->setText(cfg.lan1Ip);
        ui->lan1_mask    ->setText(cfg.lan1Netmask);
        ui->lan1_gateway ->setText(cfg.lan1Gateway);
        ui->lan1_comboBox->setCurrentIndex(cfg.lan1UseDhcp ? 1 : 0);   // 0=Static 1=DHCP
        /* ========== 速度（不可选） ========== */
        ui->lan1_speed_comboBox->setCurrentIndex(cfg.lan1Speed);   // 0=Auto 1=100M-FD ...
        //  设备速度上位机不可控制，故让其只显示
        ui->lan1_speed_comboBox->setEnabled(false);
        ui->lan1_comboBox->setEnabled(true);
        Logger::instance()->log(Logger::Info, "MainWindow", "Read Network Config");
        appendDeviceLog("读取配置成功：网络配置");
    }
    if (op == "Update device information")
    {
        Logger::instance()->log(Logger::Info, "MainWindow", "Update device information");
        ui->tableWidgetSearch->setRowCount(0);
        m_devCtl->searchDevices();
    }
    if (op == "Equipment configuration update")
    {
        Logger::instance()->log(Logger::Info, "MainWindow", "Equipment configuration update");
        appendDeviceLog("写入配置成功：网络配置");
        m_devCtl->readOverview();
    }
    if (op == "port lock/unlock")
    {
        m_devCtl->readPortLockStatus();
        QList<QCheckBox *>checkBoxs = ui->tableWidget_4->findChildren<QCheckBox *>();
        for (QCheckBox *checkBox : checkBoxs)
        {
            checkBox->setChecked(false);
        }
    }
    if (op == "Read Serial Config")
    {
        if (ui->chooseAllPortCheckBox->checkState() != Qt::Checked)
        {
            m_serialPort = m_devCtl->currentDevice()->serialPorts().first();
            ui->allPortShowTableWidget->setRowCount(1);     // 创建1行
        }
        else
        {
            m_serialPort = m_devCtl->currentDevice()->serialPorts().at(ui->comboBox_6->currentIndex());
            ui->allPortShowTableWidget->setRowCount(m_devCtl->currentDevice()->serialPorts().size());
        }

        // 从设备模型中获取最新的串口配置（使用第一个端口的配置作为示例）
        if (!m_devCtl->currentDevice()->serialPorts().isEmpty())
        {
            const QList<SerialPortConfig> configs = m_devCtl->currentDevice()->serialPorts();
            const bool showAllPorts = ui->chooseAllPortCheckBox->checkState() == Qt::Checked;
            const int displayRows = showAllPorts ? configs.size() : 1;
            for (int row = 0; row < displayRows; row++)
            {
                const int configIndex = showAllPorts ? row : ui->comboBox_6->currentIndex();
                if (configIndex < 0 || configIndex >= configs.size())
                {
                    continue;
                }
                m_serialPort = configs.at(configIndex);
                ui->allPortShowTableWidget->setItem(row, 0, new QTableWidgetItem(QString::number(m_serialPort.index)));
                ui->allPortShowTableWidget->setItem(row, 1, new QTableWidgetItem(m_serialPort.alias));
                ui->allPortShowTableWidget->setItem(row, 2, new QTableWidgetItem(QString::number(m_serialPort.baudRate)));
                ui->allPortShowTableWidget->setItem(row, 3, new QTableWidgetItem(QStringList{"", "", "", "", "", "5", "6", "7", "8"}.value(m_serialPort.dataBits)));
                ui->allPortShowTableWidget->setItem(row, 4, new QTableWidgetItem(QStringList{"", "1", "2"}.value(m_serialPort.stopBits)));
                ui->allPortShowTableWidget->setItem(row, 5, new QTableWidgetItem(QStringList{"None", "Odd", "Even", "Mark", "Space"}.value(m_serialPort.parity)));
                ui->allPortShowTableWidget->setItem(row, 6, new QTableWidgetItem(QStringList{"RS-232", "RS-422", "RS-485-2", "RS-485-4"}.value(m_serialPort.interface, "?")));
                ui->allPortShowTableWidget->setItem(row, 7, new QTableWidgetItem(QStringList{"None", "RTS/CTS", "XON/XOFF", "DTR/DSR"}.value(m_serialPort.flowControl)));
            }
            const int selectedIndex = ui->comboBox_6->currentIndex();
            if (selectedIndex >= 0 && selectedIndex < configs.size())
            {
                m_serialPort = configs.at(selectedIndex);
            }
            m_PortOldName = m_serialPort.alias;
            ui->alias_lineEdit->setText(m_serialPort.alias);
            // 校验位：0=None, 1=Odd, 2=Even, 3=Mark, 4=Space
            QString parityText = QStringList{"None", "Odd", "Even", "Mark", "Space"}.value(m_serialPort.parity, "None");
            ui->check_bit_comboBox->setCurrentText(parityText);
            // 数据位：5-8
            QString dataBitsText = QString::number(m_serialPort.dataBits);
            ui->data_bit_comboBox->setCurrentText(dataBitsText);
            // 停止位：1-2
            QString stopBitsText = QString::number(m_serialPort.stopBits);
            ui->stop_bit_comboBox->setCurrentText(stopBitsText);
            // 波特率
            QString baudRateText = QString::number(m_serialPort.baudRate);
            ui->baud_rate_comboBox->setCurrentText(baudRateText);
            QString flowControlText = QStringList{"None", "RTS/CTS", "XON/XOFF", "DTR/DSR"}.value(m_serialPort.flowControl, "None");
            ui->flow_control_comboBox->setCurrentText(flowControlText);   // 流控
            // 接口类型：0=RS-232, 1=RS-422, 2=RS-485-2, 3=RS-485-4
            QString interfaceText = QStringList{"RS-232", "RS-422", "RS-485-2", "RS-485-4"}.value(m_serialPort.interface, "RS-232");
            ui->interface_mode_comboBox->setCurrentText(interfaceText);

            m_lastSerialPortConfig = m_serialPort;
            m_hasLastSerialPortConfig = true;
            appendDeviceLog(QString("读取配置成功：串口%1配置").arg(m_serialPort.index));
        }
    }
    if (op == "Write Serial Config")
    {
        appendDeviceLog("写入配置成功：串口配置");
        m_devCtl->readSerialConfig(ui->comboBox_6->currentIndex() + 1);
    }
    if (op == "Write Mode")
    {
        appendDeviceLog("写入配置成功：端口工作模式");
    }
    if (op == "Factory Reset")
    {
        appendDeviceLog(QString("恢复出厂设置成功：%1").arg(currentCheckedDeviceText()));
    }
    if (op == "Reboot")
    {
        appendDeviceLog(QString("重启设备成功：%1").arg(currentCheckedDeviceText()));
    }
    if (op == "WatchDog")
    {
        appendDeviceLog("设置看门狗成功");
    }
    if (op == "Read Port Mode")
    {
        ui->tableWidget_7->setRowCount(1);     // 创建1行
        SerialPortMode portMode = m_devCtl->currentDevice()->seriaPortModes();
        portMode.alias = m_serialPort.alias;
        ui->tableWidget_7->setItem(0, 0, new QTableWidgetItem(QString::number(portMode.index)));
        ui->tableWidget_7->setItem(0, 1, new QTableWidgetItem(portMode.alias));
        ui->tableWidget_7->setItem(0, 2, new QTableWidgetItem(QStringList{"", "Real COM Mode", "TCP Server Mode", "Redundant Mode"}.value(portMode.workMode)));

    }
}

/* ============================================================
四、写操作（网络配置为例）
============================================================ */
void MainWindow::on_Basic_Config_Btn_clicked()
{
    NetworkConfig cfg;

    // 根据协议02-4.3.2因为01和02代表lan1和lan2---03代表lan1 lan2。所以我直接使用03,默认直接同时设置2个
    cfg.lanCount = 3;
    cfg.lan1UseDhcp = (ui->lan1_comboBox->currentText() == "DHCP");
    cfg.lan1Ip      = ui->lan1_ip   ->text();
    cfg.lan1Netmask = ui->lan1_mask ->text();
    cfg.lan1Gateway = ui->lan1_gateway->text();

    appendDeviceLog(QString("写入配置：网络配置，%1").arg(currentCheckedDeviceText()));
    if (m_hasLastNetworkConfig)
    {
        const QStringList changes = networkConfigChanges(m_lastNetworkConfig, cfg);
        if (changes.isEmpty())
        {
            appendDeviceLog("配置修改：网络配置未发生变化");
        }
        else
        {
            for (const QString &change : changes)
            {
                appendDeviceLog(QString("配置修改：%1").arg(change));
            }
        }
    }
    else
    {
        appendDeviceLog("配置修改：未读取到旧网络配置，无法比较修改项");
    }
    
    Logger::instance()->log(Logger::Info, "MainWindow", QString("正在写入网络配置：LAN1=%1/%2/%3 (DHCP:%4), LAN2=%5/%6/%7 (DHCP:%8)").arg(cfg.lan1Ip).arg(cfg.lan1Netmask).arg(cfg.lan1Gateway).arg(cfg.lan1UseDhcp).arg(cfg.lan2Ip).arg(cfg.lan2Netmask).arg(cfg.lan2Gateway).arg(cfg.lan2UseDhcp));
    
    m_devCtl->writeNetworkConfig(cfg);
    m_lastNetworkConfig = cfg;
    m_hasLastNetworkConfig = true;
    Logger::instance()->log(Logger::Debug, "MainWindow", "网络配置写入命令已发送");
}

void MainWindow::onOperationFailed(const QString &op, const QString &err)
{
    QMessageBox::warning(this, op + "失败", err);
    Logger::instance()->log(Logger::Error, "MainWindow", QString("操作失败: %1, 错误: %2").arg(op).arg(err));
    appendDeviceLog(QString("操作失败：%1，原因：%2").arg(op, err));
}

/* ============================================================
五、其余 UI 槽函数（串口、端口、固件等）套路完全一致：
构造 Model 对象 → 调用 Controller → 等待 success/failed 信号
代码过多，这里直接留空，按上面模式补全即可。
============================================================ */
void MainWindow::on_reset_btn_clicked()
{
    /* 同步位图（固定 32 字节，checkBox_56-71 → 端口1-16） *****/
    QByteArray sync(16, 0);        // 默认全 0
    int        used = 0;           // 实际写入字节数

    /* 统一规则：对象名 "checkBox_56"~"checkBox_71" 且被勾选 → 按出现顺序写入端口号 */
    const QList<QCheckBox*> boxes = this->findChildren<QCheckBox*>(QRegularExpression("checkBox_(5[6-9]|6[0-9]|71)"));
    for (QCheckBox *cb : boxes)
    {
        if (!cb->isChecked()) continue;
        int port = -1;
        QRegularExpression re("checkBox_(\\d+)");
        QRegularExpressionMatch match = re.match(cb->objectName());
        if (match.hasMatch())
        {
            int num = match.captured(1).toInt();
            if (num >= 56 && num <= 71) port = num - 55;   // 56→1, 71→16
        }
        if (port < 1 || port > 16) continue;
        if (used >= 16) break;
        sync[used++] = static_cast<quint8>(port);
    }

    // 获取选中的个数和端口号
    QByteArray checkNumPort = QByteArray::fromHex((QString("%1").arg(used, 2, 16, QChar('0'))).toUtf8());
    QByteArray portNum = sync.left(used);
    checkNumPort.append(portNum);

    /* 5. 把 32 字节位图追加到同一个命令帧（协议可选段） *********/
    m_devCtl->buildPortReset(checkNumPort);   // 你已经在 DeviceController 里实现好的接口
}


void MainWindow::on_self_check_btn_clicked()
{ 
    /* 同步位图（固定 32 字节，checkBox_56-71 → 端口1-16） *****/
    QByteArray sync(16, 0);        // 默认全 0
    int        used = 0;           // 实际写入字节数

    /* 统一规则：对象名 "checkBox_56"~"checkBox_71" 且被勾选 → 按出现顺序写入端口号 */
    const QList<QCheckBox*> boxes = this->findChildren<QCheckBox*>(QRegularExpression("checkBox_(5[6-9]|6[0-9]|71)"));
    for (QCheckBox *cb : boxes)
    {
        if (!cb->isChecked()) continue;
        int port = -1;
        QRegularExpression re("checkBox_(\\d+)");
        QRegularExpressionMatch match = re.match(cb->objectName());
        if (match.hasMatch())
        {
            int num = match.captured(1).toInt();
            if (num >= 56 && num <= 71) port = num - 55;   // 56→1, 71→16
        }
        if (port < 1 || port > 16) continue;
        if (used >= 16) break;
        sync[used++] = static_cast<quint8>(port);
    }

    // 获取选中的个数和端口号
    QByteArray checkNumPort = QByteArray::fromHex((QString("%1").arg(used, 2, 16, QChar('0'))).toUtf8());
    QByteArray portNum = sync.left(used);
    checkNumPort.append(portNum);

    /* 5. 把 32 字节位图追加到同一个命令帧（协议可选段） *********/
    m_devCtl->appendSyncBitmap(checkNumPort);   // 你已经在 DeviceController 里实现好的接口
}

void MainWindow::on_duan_can_btn_clicked()
{
    /* 1. 取出当前要设置的端口编号 */
    QString portText = ui->comboBox_6->currentText();   // 例如 "端口6"
    if (portText.isEmpty())
    {
        QMessageBox::warning(this, "提示", "请选择要设置的串口");
        return;
    }
    int port = portText.mid(2).toInt();               // "端口1" → 1
    if (port < 1 || port > 16)
    {
        QMessageBox::warning(this, "提示", "端口号必须在 1~16 之间");
        return;
    }
    if (ui->alias_lineEdit->text().length() > 19)
    {
        QMessageBox::warning(this, "提示", "端口别名不能大于19字符");
        return;
    }
    if (ui->alias_lineEdit->text() != m_PortOldName && ui->PortNewNameCBox->checkState() != Qt::Checked)
    {
        QMessageBox::information(this, "提示", "请勾选复选框修改别名");
        return;
    }
    /* 2. 从 UI 读取所有参数 */
    SerialPortConfig cfg;
    cfg.index       = port;     //1
    if (ui->PortNewNameCBox->checkState() != Qt::Checked || ui->alias_lineEdit->text().isEmpty())
    {
        cfg.alias = m_PortOldName;
    }
    else
    {
        cfg.alias = ui->alias_lineEdit->text();
    }
    cfg.baudRate = ui->baud_rate_comboBox->currentText().toUInt();
    cfg.dataBits = ui->data_bit_comboBox->currentText().toUInt();
    cfg.stopBits = ui->stop_bit_comboBox->currentText().toUInt();

    /* 校验位 combo 文本 → 0/1/2 */
    QString parText = ui->check_bit_comboBox->currentText();
    cfg.parity = (parText == "None") ? 0 : (parText == "Odd") ? 1 : (parText == "Even") ? 2 : (parText == "Mark") ? 3 : 4 ;

    /* 流控 combo 文本 → 0/1/2/3 */
    QString flowText= ui->flow_control_comboBox->currentText();
    cfg.flowControl = (flowText == "None") ? 0 : (flowText == "RTS/CTS") ? 1 : (flowText == "XON/XOFF") ? 2 : 3;
    cfg.fifo = 0;   // 0 表示开启 FIFO（界面未提供则默认）

    /* 接口类型 combo 文本 → 0/1/2/3 */
    QString ifText  = ui->interface_mode_comboBox->currentText();
    cfg.interface = (ifText == "RS-232") ? 0 : (ifText == "RS-422") ? 1 : (ifText == "RS-485-2") ? 2 : 3;

    appendDeviceLog(QString("写入配置：串口%1配置，%2").arg(port).arg(currentCheckedDeviceText()));
    if (m_hasLastSerialPortConfig && m_lastSerialPortConfig.index == cfg.index)
    {
        const QStringList changes = serialConfigChanges(m_lastSerialPortConfig, cfg);
        if (changes.isEmpty())
        {
            appendDeviceLog(QString("配置修改：串口%1配置未发生变化").arg(port));
        }
        else
        {
            for (const QString &change : changes)
            {
                appendDeviceLog(QString("配置修改：串口%1 %2").arg(port).arg(change));
            }
        }
    }
    else
    {
        appendDeviceLog(QString("配置修改：未读取到串口%1旧配置，无法比较修改项").arg(port));
    }

    Logger::instance()->log(Logger::Info, "MainWindow", QString("正在设置串口 %1：波特率=%2, 数据位=%3, 停止位=%4, 校验位=%5, 流控=%6, 接口类型=%7, 别名=%8").arg(port).arg(cfg.baudRate).arg(cfg.dataBits).arg(cfg.stopBits).arg(parText).arg(flowText).arg(ifText).arg(cfg.alias));

    /* 11. 同步位图（可选） *************************************/
    if (ui->checkBox_33->isChecked())
    {
        // 关键1：初始化时直接创建32字节全0的数组，而非reserve后补0
        QByteArray sync(32, 0); // 直接分配32字节，全部初始化为0（彻底避免垃圾值）
        int writeIndex = 0;     // 记录写入位置，确保按顺序写
        int writeIndex_2 = 0;
        // 规则：checkBox_17~32 → 端口1~16
        const int CB_NUM_START = 17;
        const int CB_NUM_END = 32;
        const int PORT_OFFSET = 16;

        for (int cbNum = CB_NUM_START; cbNum <= CB_NUM_END; ++cbNum)
        {
            QCheckBox *cb = findChild<QCheckBox*>(QString("checkBox_%1").arg(cbNum));

            // 只处理选中的端口，且写入位置不超过31（0~31共32字节）
            if (cb && cb->isChecked() && writeIndex < 32)
            {
                int port = cbNum - PORT_OFFSET;
                QByteArray portBytes = QByteArray::fromHex((QString("%1").arg(port, 2, 16, QChar('0'))).toUtf8());
                if (!portBytes.isEmpty()) {
                    sync[writeIndex++] = portBytes.at(0);
                }
            }
        }

        writeIndex_2 = writeIndex;
        QByteArray hexString = QByteArray::fromHex((QString("%1").arg(writeIndex_2, 2, 16, QChar('0'))).toUtf8());      //返回设置的个数
        if (!hexString.isEmpty()) {
            QByteArray result = sync.left(writeIndex_2);    //返回实际被选择的端口号
            hexString.append(result);
        }

        // 验证：打印最终32字节数据，确保只有前writeIndex个字节是端口号，其余全0
        Logger::instance()->log(Logger::Debug, "MainWindow", QString("最终32字节同步数据：%1").arg(QString(sync.toHex(' ').toUpper())));

        // 追加到cmd前，确保cmd是干净的（可选：如果cmd有残留，先清空）
        // cmd.clear(); // 若cmd之前有数据，建议先清空
        cfg.syncBitmap = hexString;
        const QStringList syncPortNames = lockPortNamesFromPayload(cfg.syncBitmap);
        appendDeviceLog(QString("启用同步：将串口%1配置同步到%2")
                        .arg(port)
                        .arg(syncPortNames.isEmpty() ? "未选择端口" : syncPortNames.join("，")));
        /* 3. 调用新架构写串口配置 */
        Logger::instance()->log(Logger::Debug, "MainWindow", "带同步位图的串口配置写入命令已发送");
        m_devCtl->writeSerialConfig(cfg,0);   // 内部会组帧 0x04 0x03 并等待结果
    }
    else
    {
        appendDeviceLog(QString("未启用同步：仅写入串口%1配置").arg(port));
        /* 3. 调用新架构写串口配置 */
        Logger::instance()->log(Logger::Debug, "MainWindow", "普通串口配置写入命令已发送");
        m_devCtl->writeSerialConfig(cfg,1);   // 内部会组帧 0x04 0x02 并等待结果
    }
}

/* ----------------------------------------------------------
 * 工作模式设置
 * ---------------------------------------------------------- */
void MainWindow::on_work_mode_btn_clicked()
{
    QString portText = ui->comboBox_7->currentText();          // 下拉框，选择某一个端口1～16
    if (portText.isEmpty())
    {
        QMessageBox::warning(this, "提示", "请选择要设置的串口");
        return;
    }
    int port = portText.mid(2).toInt();
    if (port < 1 || port > 16)
    {
        return;
    }

    /* 读取界面工作模式 */
    QString modeText = ui->work_mode_comboBox->currentText();

    quint8 mode = 0x01;   // 默认为Real COM Mode模式
    if (modeText == "TCP Server Mode")
    {
        mode = 0x03;
    }
    else if (modeText == "Redundant Mode")
    {
        mode = 0x07;
    }

    /* 构造模式参数 */
    QVariantMap params;
    if (mode == 0x01)
    {
        params["RealComModeTCPaliveCheckTime"] = ui->spinBox_3->value();     // TCP活动检测时间
        params["RealComModeTCPmaxConnectNum"] = ui->spinBox_2->value();    // TCP最大连接数
    }

    if (mode == 0x03)
    {
        params["TCPServerModeTCPaliveCheckTime"] = ui->spinBox->value();      // TCP活动检测时间
        params["TCPServerTCPmaxConnectNum"]    = ui->spinBox_7->value();      // TCP最大连接数
    }
    if (mode == 0x05) params["group"] = ui->spinBox_4->value();            // Redundant 组号

    Logger::instance()->log(Logger::Info, "MainWindow", QString("正在设置端口 %1 的工作模式：%2").arg(port).arg(modeText));
    appendDeviceLog(QString("写入配置：端口%1工作模式，模式=%2").arg(port).arg(modeText));
    if (params.isEmpty())
    {
        appendDeviceLog(QString("配置修改：端口%1工作模式 -> %2").arg(port).arg(modeText));
    }
    else
    {
        QStringList detailList;
        for (auto it = params.constBegin(); it != params.constEnd(); ++it)
        {
            detailList << QString("%1=%2").arg(it.key(), it.value().toString());
        }
        appendDeviceLog(QString("配置修改：端口%1工作模式 -> %2，参数：%3").arg(port).arg(modeText).arg(detailList.join("，")));
    }

    /* 4. 同步端口列表（可选，固定 32 字节） */
    SerialPortConfig cfg;
    if (ui->checkBox_34->isChecked())
    {
        if(mode == 0x01)
        {
            mode = 0x02;
        }
        else if(mode == 0x03)
        {
            mode = 0x04;
        }

        // 关键1：初始化时直接创建32字节全0的数组，而非reserve后补0
        QByteArray sync(32, 0); // 直接分配32字节，全部初始化为0（彻底避免垃圾值）
        int writeIndex = 0;     // 记录写入位置，确保按顺序写
        int writeIndex_2 = 0;

        const int CB_NUM_START = 35;    //从第35个端口号开始------即端口1
        const int CB_NUM_END = 50;      //到第50个端口结束 ------即16个端口

        for (int cbNum = CB_NUM_START; cbNum <= CB_NUM_END; ++cbNum)
        {
            QCheckBox *cb = findChild<QCheckBox*>(QString("checkBox_%1").arg(cbNum));
            if(cb && cb->isChecked() && writeIndex < 32)
            {
                int i = cbNum - 34;
                sync[writeIndex++] = QByteArray::fromHex((QString("%1").arg(i, 2, 16, QChar('0'))).toUtf8()).at(0);
            }
        }
        writeIndex_2 = writeIndex;

        QByteArray hexString = QByteArray::fromHex((QString("%1").arg(writeIndex_2, 2, 16, QChar('0'))).toUtf8());      //返回实际选择的个数
        QByteArray result = sync.left(writeIndex);      //只返回实际使用的端口号

        hexString.append(result);

        // 追加到cmd前，确保cmd是干净的（可选：如果cmd有残留，先清空）
        // cmd.clear(); // 若cmd之前有数据，建议先清空
        cfg.syncBitmap = hexString;     //hexString 包含 1 + n ： 个数 + 端口号
        const QStringList syncPortNames = lockPortNamesFromPayload(cfg.syncBitmap);
        appendDeviceLog(QString("启用同步：将端口%1工作模式同步到%2")
                        .arg(port)
                        .arg(syncPortNames.isEmpty() ? "未选择端口" : syncPortNames.join("，")));
        Logger::instance()->log(Logger::Debug, "MainWindow", "带同步端口列表的工作模式设置命令已发送");
        m_devCtl->writeModeConfig(port, mode, params, cfg, 0);
    }
    else
    {
        appendDeviceLog(QString("未启用同步：仅写入端口%1工作模式").arg(port));
        Logger::instance()->log(Logger::Debug, "MainWindow", "checkBox_34未选中，跳过同步端口列表");
        m_devCtl->writeModeConfig(port, mode, params, cfg, 1);   // Controller 内部组帧 0x05 0x01
    }

}

/* ----------------------------------------------------------
 * 端口镜像
 * ---------------------------------------------------------- */
void MainWindow::on_use_monitoring_btn_4_clicked()
{
    bool enable = ui->checkBox_74->isChecked();
    QString srcText = ui->comboBox_12->currentText();   // 源端口
    QString dstText = ui->comboBox_13->currentText();   // 目标端口
    int src = srcText.mid(2).toInt();
    int dst = dstText.mid(2).toInt();

    if (src < 1 || src > 16 || dst < 1 || dst > 16) return;

    QByteArray pkt;
    pkt.append(static_cast<char>(enable ? 0x01 : 0x00));
    pkt.append(static_cast<char>(src));
    pkt.append(static_cast<char>(dst));

    appendDeviceLog(QString("设置端口镜像：状态=%1，源端口=端口%2，目标端口=端口%3")
                    .arg(enable ? "启用" : "禁用")
                    .arg(src)
                    .arg(dst));
    m_devCtl->setPortMirror(pkt);   // Controller 封装 0x08 0x05
}

/* ----------------------------------------------------------
 * 设备锁定
 * ---------------------------------------------------------- */
void MainWindow::on_lock_btn_clicked()
{
    if (QMessageBox::question(this, "确认", "锁定后需要解锁码才能解锁，继续？", QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) != QMessageBox::Yes) return;
    Logger::instance()->log(Logger::Info, "MainWindow", "正在锁定设备");
    appendDeviceLog(QString("请求锁定设备：%1").arg(currentCheckedDeviceText()));
    m_devCtl->lockDevice();   // 组帧 0x08 0x06
}

/* ----------------------------------------------------------
 * 设备解锁
 * ---------------------------------------------------------- */
void MainWindow::on_unlock_btn_clicked()
{
    QString code = QString(ui->lineEdit->text().trimmed());
    Logger::instance()->log(Logger::Info, "MainWindow", QString("正在解锁设备，解锁码: %1").arg(code));
    if (code.size() != 8)
    {
        QMessageBox::warning(this, "提示", "解锁码必须为 8 位");
        Logger::instance()->log(Logger::Warning, "MainWindow", "解锁码长度不正确");
        appendDeviceLog(QString("操作失败：解锁码长度不正确，当前长度=%1").arg(code.size()));
        return;
    }
    appendDeviceLog(QString("请求解锁设备：%1，解锁码=%2").arg(currentCheckedDeviceText(), code));
    m_devCtl->unlockDevice(code);   // 组帧 0x08 0x07
}

/* ----------------------------------------------------------
 * 看门狗
 * ---------------------------------------------------------- */
void MainWindow::on_watchdog_btn_clicked()
{
    bool enable = ui->checkBox_55->isChecked();
    if (enable)
    {
        int ret = QMessageBox::warning(this, "确认",
                                       "启用看门狗后若未按时喂狗，设备将循环重启！",
                                       QMessageBox::Yes | QMessageBox::No,
                                       QMessageBox::No);
        if (ret != QMessageBox::Yes) return;
    }
    quint8 enableByte = enable ? 0x01 : 0x00;
    quint16 timeout   = static_cast<quint16>(ui->spinBox_5->value());
    QByteArray pkt;
    pkt.append(static_cast<char>(enableByte));
    pkt.append(static_cast<char>(timeout >> 8));
    pkt.append(static_cast<char>(timeout & 0xFF));
    Logger::instance()->log(Logger::Info, "MainWindow", QString("正在设置看门狗：状态=%1, 超时时间=%2秒").arg(enable ? "启用" : "禁用").arg(timeout));
    appendDeviceLog(QString("设置看门狗：状态=%1，超时时间=%2秒").arg(enable ? "启用" : "禁用").arg(timeout));
    m_devCtl->setWatchDog(pkt);   // 组帧 0x08 0x09
}

/* ----------------------------------------------------------
 * 恢复出厂设置
 * ---------------------------------------------------------- */
void MainWindow::on_factory_reset_btn_clicked()
{
    if (QMessageBox::warning(this, "确认", "所有配置将丢失！继续？",
                             QMessageBox::Yes | QMessageBox::No,
                             QMessageBox::No) != QMessageBox::Yes) return;
    bool ok = false;
    QString key = QInputDialog::getText(this, "最终确认",
                                        "请输入 ERASE 以确认：",
                                        QLineEdit::Normal, "", &ok);
    if (!ok || key != "ERASE") return;
    Logger::instance()->log(Logger::Info, "MainWindow", "正在恢复设备出厂设置");
    appendDeviceLog(QString("请求恢复出厂设置：%1").arg(currentCheckedDeviceText()));
    m_devCtl->factoryReset();   // 组帧 0x08 0x0A
}

/* ----------------------------------------------------------
 * 重启设备
 * ---------------------------------------------------------- */
void MainWindow::on_restart_btn_clicked()
{
    int ret = QMessageBox::question(this, "确认", "设备将立即重启，继续？",
                                    QMessageBox::Yes | QMessageBox::No,
                                    QMessageBox::No);
    if (ret == QMessageBox::Yes)
    {
        Logger::instance()->log(Logger::Info, "MainWindow", "正在重启设备");
        appendDeviceLog(QString("请求重启设备：%1").arg(currentCheckedDeviceText()));
        m_devCtl->reboot();   // 组帧 0x08 0x0D
    }
}

/* ----------------------------------------------------------
 * 固件升级
 * ---------------------------------------------------------- */
void MainWindow::on_upgrade_btn_clicked()
{
    QString file = ui->lineEdit_fwFile->text().trimmed();
    if (file.isEmpty())
    {
        QMessageBox::warning(this, "提示", "请先选择固件文件！");
        return;
    }

    /* 第一重：普通确认 */
    int ret = QMessageBox::question(this, "确认升级",
                                    "升级过程中绝对不能断电或断开连接！\n您确定要升级吗？",
                                    QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (ret != QMessageBox::Yes) return;

    if (file.endsWith(".pkg", Qt::CaseInsensitive))
    {
        Logger::instance()->log(Logger::Info, "MainWindow", QString("开始升级固件，文件路径: %1").arg(file));
        appendDeviceLog(QString("开始固件升级：文件=%1").arg(file));
        startPackageUpgrade(file);
    }
    else
    {
        QMessageBox::warning(this, "固件升级", "请选择正确的.pkg文件");
        Logger::instance()->log(Logger::Error, "MainWindow", "固件升级失败：请选择正确的.pkg文件");
        appendDeviceLog(QString("操作失败：固件升级文件格式不正确，文件=%1").arg(file));
    }
}

/* ----------------------------------------------------------
 * 浏览固件文件
 * ---------------------------------------------------------- */
void MainWindow::on_browse_btn_clicked()
{
    QString file = QFileDialog::getOpenFileName(
                this, "选择固件", QString(),
                "固件包 (*.pkg);;固件文件 (*.bin *.hex);;所有文件 (*)");
    if (!file.isEmpty())
    {
        ui->lineEdit_fwFile->setText(file);
        ui->upgrade_btn->setText(file.endsWith(".pkg", Qt::CaseInsensitive) ? "开始升级" : "开始升级");
    }
}
/* ----------------------------------------------------------
 * 端口锁定（多选）
 * ---------------------------------------------------------- */
void MainWindow::on_Port_lockout_btn_clicked()
{
    QByteArray selected = collectSelectedLockPorts(1);
    QStringList portNames = lockPortNamesFromPayload(selected);
    if (portNames.isEmpty())
    {
        QMessageBox::information(this, "提示", "请先勾选要锁定的端口");
        appendDeviceLog("操作失败：请先勾选要锁定的端口，或所选端口已处于锁定状态");
        return;
    }

    m_pendingLockPortNames = portNames;
    appendDeviceLog(QString("请求锁定端口：%1").arg(portNames.join("，")));

    /* 追加 32 字节同步位图 → 同一个命令帧 */
    m_devCtl->lockPortsWithSync(selected);   // 新接口，见第 5 步
}

/* ----------------------------------------------------------
 * 端口解锁（多选）
 * ---------------------------------------------------------- */
void MainWindow::on_Port_unlock_btn_clicked()
{
    QByteArray selected = collectSelectedLockPorts(2);
    QStringList portNames = lockPortNamesFromPayload(selected);
    if (portNames.isEmpty()) {
        QMessageBox::information(this, "提示", "请先勾选要解锁的端口");
        appendDeviceLog("操作失败：请先勾选要解锁的端口，或所选端口已处于未锁定状态");
        return;
    }
    m_pendingUnlockPortNames = portNames;
    appendDeviceLog(QString("请求解锁端口：%1").arg(portNames.join("，")));
    m_devCtl->unlockPortsWithSync(selected);
}

void MainWindow::on_pushButton_clicked()
{
    ui->stackedWidget->setCurrentIndex(0);
};


void MainWindow::on_pushButton_2_clicked()         { ui->tableWidgetSearch->clearContents(); };
void MainWindow::on_pushButton_3_clicked()
{
    ui->stackedWidget->setCurrentIndex(0);

};
void MainWindow::on_pushButton_4_clicked()
{
#if defined(Q_OS_WIN)
    /* Windows：查注册表 */
    QSettings reg("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\NPort",
                  QSettings::NativeFormat);
    bool installed = reg.contains("DisplayName");   // MOXA 服务存在即认为驱动装好
#elif defined(Q_OS_LINUX)
    /* Linux：查 systemd 服务或 /lib/modules/$(uname -r)/extra/nport*.ko */
    bool installed = QFile::exists("/lib/modules/" + QSysInfo::kernelVersion() +
                                   "/extra/moxa/nport.ko");
#else
    bool installed = false;
#endif

    QMessageBox::information(
                this,
                "驱动检测",
                installed ? "✅ MOXA NPort 驱动已安装！" : "❌ 驱动未安装，请先安装驱动。"
                            );
};

//串口映射按钮
void MainWindow::on_pushButton_5_clicked()
{
    // 单例模式：只创建一个实例
    if (!m_configDialog)
    {
        m_configDialog = new ConfigDialog(this);

        // 对话框关闭时的处理（可选）
        connect(m_configDialog, &QDialog::finished, [this]()
        {
            // 可以选择保留对话框实例或删除
            // m_configDialog->deleteLater();
            // m_configDialog = nullptr;
        });
    }

    // 显示对话框
    m_configDialog->show();
    m_configDialog->raise();
    m_configDialog->activateWindow();
};

void MainWindow::on_pushButton_10_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
    ui->stackedWidget_2->setCurrentIndex(4);
    ui->tabWidget->setCurrentIndex(0);
    on_tabWidget_currentChanged(0);
    ui->checkBox_35->setChecked(true);
};

// 修改设备名称
void MainWindow::on_name_modify_clicked()
{
    QString newName = ui->new_name->text().trimmed();
    if(newName.isEmpty())
    {
        QMessageBox::warning(this, "提示", "设备名称不能为空");
        Logger::instance()->log(Logger::Warning, "MainWindow", "设备名称为空");
        return;
    }
    if(newName.length() > 20)
    {
        QMessageBox::warning(this, "提示", "设备名称不能超过 20 个字符");
        Logger::instance()->log(Logger::Warning, "MainWindow", "设备名称长度超过20个字符");
        return;
    }

    QString oldName;
    if (m_devCtl && m_devCtl->currentDevice())
    {
        oldName = m_devCtl->currentDevice()->deviceName();
    }
    appendDeviceLog(QString("写入配置：设备名称，%1").arg(currentCheckedDeviceText()));
    if (!oldName.isEmpty() && oldName != newName)
    {
        appendDeviceLog(QString("配置修改：设备名称：%1 -> %2").arg(oldName, newName));
    }
    else if (oldName == newName)
    {
        appendDeviceLog("配置修改：设备名称未发生变化");
    }
    else
    {
        appendDeviceLog(QString("配置修改：设备名称 -> %1").arg(newName));
    }

    Logger::instance()->log(Logger::Info, "MainWindow", QString("正在修改设备名称为：%1").arg(newName));
    /* 调用新架构接口 */
    m_devCtl->writeDeviceName(newName);   // 内部组帧 0x02 0x01
}

void MainWindow::on_device_manage_btn_clicked()
{
    ui->stackedWidget_2->setCurrentIndex(2);
    ui->stackedWidget_4->setCurrentIndex(1);   // 日志子页
}

void MainWindow::lockorunlock()
{
    /*----------------- 根据设备状态来设置lineEdit的状态 ------------------*/
    if (UdpDiscovery::dev.devLockState == QPORT_DeviceLockState::Locked)
    {
        ui->lineEdit->setEnabled(true);
        ui->unlockPassWord_lab->setText("请在设备上查看解锁密码");
    }
    ui->lineEdit->setEnabled(false);
    ui->stackedWidget_2->setCurrentIndex(2);   // 设备管理页
    ui->stackedWidget_4->setCurrentIndex(0);   // 锁定子页
}


void MainWindow::on_pushButton_9_clicked()
{
    QMenu *menu = new QMenu(this);

    menu->addAction("端口自检/重置/镜像", this, &MainWindow::Self_Check);
    menu->addAction("端口锁定", this, &MainWindow::portLockout);
    menu->addAction("端口连接信息", this, &MainWindow::link_info);
    menu->addAction("端口状态信息", this, &MainWindow::Status_information);
    menu->addAction("端口错误信息", this, &MainWindow::Error_Message);
    menu->addAction("端口日志记录", this, &MainWindow::port_log);

    menu->exec(QCursor::pos());
}


void MainWindow::port_log()
{

}

/* ----------------------------------------------------------
 * 端口自检
 * ---------------------------------------------------------- */
void MainWindow::Self_Check()
{
    ui->stackedWidget_2->setCurrentIndex(3);
    ui->stacked_port->setCurrentIndex(1);   // 自检结果页
}

/* ----------------------------------------------------------
 * 端口锁定（多选）
 * ---------------------------------------------------------- */
void MainWindow::portLockout()
{
    ui->stackedWidget_2->setCurrentIndex(3);
    ui->stacked_port->setCurrentIndex(0);   // 端口锁定页
    m_devCtl->readPortLockStatus();   // ← 一键查询
    QList<QCheckBox *>checkBoxs = ui->tableWidget_4->findChildren<QCheckBox *>();
    for (QCheckBox *checkBox : checkBoxs)
    {
        checkBox->setChecked(false);
    }
}

/* ----------------------------------------------------------
 * 端口连接信息
 * ---------------------------------------------------------- */
void MainWindow::link_info()
{
    m_devCtl->readPortConnInfo();   // 0x07 0x01
    ui->stackedWidget_2->setCurrentIndex(3);
    ui->stacked_port->setCurrentIndex(2);
}

/* ----------------------------------------------------------
 * 端口状态信息
 * ---------------------------------------------------------- */
void MainWindow::Status_information()
{
    m_devCtl->readPortStatus();     // 0x07 0x02
    ui->stackedWidget_2->setCurrentIndex(3);
    ui->stacked_port->setCurrentIndex(3);
}

/* ----------------------------------------------------------
 * 端口错误信息
 * ---------------------------------------------------------- */
void MainWindow::Error_Message()
{
    m_devCtl->readPortError();      // 0x07 0x03
    ui->stackedWidget_2->setCurrentIndex(3);
    ui->stacked_port->setCurrentIndex(4);
}

/* 进度条前进（0~total） */
void MainWindow::onUpgradeProgress(int ack)
{
    if (m_progress) m_progress->setValue(ack);
}

/* 成功收尾 */
void MainWindow::onUpgradeSuccess()
{
    if (m_mask)
    {
        m_mask->hide();
        QMessageBox::information(this, "升级成功", "固件包升级完成！设备即将重启，请稍后重新连接。");
    }
    //    upgradeFinish();
    m_devCtl->disconnectUpdate();
    m_devCtl->disconnectFromDevice();   // ✅ 通过 Controller 断开
    m_devCtl->reboot();     // 重启设备
    Logger::instance()->log(Logger::Info, "MainWindow", "固件升级成功，设备即将重启");
    appendDeviceLog("固件升级成功：设备即将重启");
}

/* 失败收尾 */
void MainWindow::onUpgradeFailed(const QString &err)
{
    if (m_mask) m_mask->hide();
    QMessageBox::critical(this, "升级失败", err);
    upgradeFinish();
    Logger::instance()->log(Logger::Error, "MainWindow", QString("固件升级失败: %1").arg(err));
    appendDeviceLog(QString("操作失败：固件升级失败，原因：%1").arg(err));
}

// 构建进度条和遮罩
void MainWindow::showUpgradeMask(int totalPkts)
{
    if (m_mask == nullptr)
    {
        m_mask = new QWidget(this);
        m_mask->setStyleSheet("background:rgba(0, 0, 0, 0.5);");
        auto *lab = new QLabel("正在写入 FLASH...", m_mask);
        lab->setStyleSheet("color:white;font:18px;");
        lab->setAlignment(Qt::AlignCenter);
        auto *lay = new QVBoxLayout(m_mask);
        lay->addWidget(lab);
        m_progress = new QProgressBar(m_mask);
        m_progress->setFormat("%p%"); // 显示百分比
        //        m_progress->setStyleSheet("QProgressBar { color: white; } QProgressBar::chunk { background-color: #4CAF50; }"); // 设置文字为白色
        lay->addWidget(m_progress);
    }
    m_mask->setGeometry(rect());
    m_progress->setRange(0, 100); // 设置范围为0-100
    m_mask->show();
}

/* 统一清理 */
void MainWindow::upgradeFinish()
{
    if (m_progress)
    {
        delete m_progress;
        m_progress = nullptr;
    }
    for (auto btn : findChildren<QPushButton *>()) btn->setEnabled(true);
    ui->upgrade_btn->setText("开始升级");
}

QString MainWindow::getDeviceStatusName(QPORT_DeviceState st)
{
    switch (st)
    {
    case QPORT_DeviceState::FactoryMode:  return "出厂模式";
    case QPORT_DeviceState::WorkingMode:  return "工作模式";
    case QPORT_DeviceState::NetworkError: return "网络故障";
    case QPORT_DeviceState::SerialError:  return "串口故障";
    case QPORT_DeviceState::SystemError:  return "系统故障";
    default:                              return "未知";
    }
}

QString MainWindow::deviceStateText(quint8 st)
{
    switch (st)
    {
    case 0x01: return QStringLiteral("出厂模式");
    case 0x02: return QStringLiteral("工作模式");
    case 0x03: return QStringLiteral("网络故障");
    case 0x04: return QStringLiteral("串口故障");
    case 0x05: return QStringLiteral("系统故障");
    default:   return QStringLiteral("未知状态");
    }
}

void MainWindow::appendDeviceLog(const QString &message)
{
    Logger::instance()->log(Logger::Info, "设备日志", message);

    if (!ui)
    {
        return;
    }

    const QString line = QString("[%1] %2")
                             .arg(QTime::currentTime().toString("HH:mm:ss"))
                             .arg(message);
    QObject *logWidget = findChild<QObject*>("devLogTextEdit");
    if (!logWidget)
    {
        return;
    }

    if (!QMetaObject::invokeMethod(logWidget, "append", Q_ARG(QString, line)))
    {
        QMetaObject::invokeMethod(logWidget, "appendPlainText", Q_ARG(QString, line));
    }
}

QString MainWindow::searchTableDeviceText(int row) const
{
    auto textAt = [this, row](int column) -> QString {
        QTableWidgetItem *item = ui->tableWidgetSearch->item(row, column);
        return item ? item->text().trimmed() : QString();
    };

    return QString("设备%1：型号=%2，名称=%3，序列号=%4，IP=%5，MAC=%6")
            .arg(row + 1)
            .arg(textAt(2))
            .arg(textAt(4))
            .arg(textAt(3))
            .arg(textAt(7))
            .arg(textAt(8));
}

QString MainWindow::currentCheckedDeviceText() const
{
    for (int row = 0; row < ui->tableWidgetSearch->rowCount(); ++row)
    {
        QTableWidgetItem *item = ui->tableWidgetSearch->item(row, 0);
        if (item && item->checkState() == Qt::Checked)
        {
            return searchTableDeviceText(row);
        }
    }

    if (!UdpDiscovery::dev.lan1Ip.isEmpty() || !UdpDiscovery::dev.lan1Mac.isEmpty())
    {
        return QString("设备：型号=%1，名称=%2，序列号=%3，IP=%4，MAC=%5")
                .arg(UdpDiscovery::dev.deviceModel)
                .arg(UdpDiscovery::dev.deviceName)
                .arg(UdpDiscovery::dev.serialNum)
                .arg(UdpDiscovery::dev.lan1Ip)
                .arg(UdpDiscovery::dev.lan1Mac);
    }

    return "当前设备";
}

QStringList MainWindow::networkConfigChanges(const NetworkConfig &oldCfg, const NetworkConfig &newCfg) const
{
    QStringList changes;
    auto addChange = [&changes](const QString &name, const QString &oldValue, const QString &newValue) {
        if (oldValue != newValue)
        {
            changes << QString("%1：%2 -> %3").arg(name, oldValue, newValue);
        }
    };

    addChange("LAN1 DHCP", oldCfg.lan1UseDhcp ? "启用" : "禁用", newCfg.lan1UseDhcp ? "启用" : "禁用");
    addChange("LAN1 IP", oldCfg.lan1Ip, newCfg.lan1Ip);
    addChange("LAN1 子网掩码", oldCfg.lan1Netmask, newCfg.lan1Netmask);
    addChange("LAN1 网关", oldCfg.lan1Gateway, newCfg.lan1Gateway);

    return changes;
}

QStringList MainWindow::serialConfigChanges(const SerialPortConfig &oldCfg, const SerialPortConfig &newCfg) const
{
    QStringList changes;
    auto textFromList = [](const QStringList &texts, int index, const QString &fallback) {
        return texts.value(index, fallback);
    };
    auto addChange = [&changes](const QString &name, const QString &oldValue, const QString &newValue) {
        if (oldValue != newValue)
        {
            changes << QString("%1：%2 -> %3").arg(name, oldValue, newValue);
        }
    };

    addChange("端口别名", oldCfg.alias, newCfg.alias);
    addChange("波特率", QString::number(oldCfg.baudRate), QString::number(newCfg.baudRate));
    addChange("数据位", QString::number(oldCfg.dataBits), QString::number(newCfg.dataBits));
    addChange("停止位", QString::number(oldCfg.stopBits), QString::number(newCfg.stopBits));
    addChange("校验位",
              textFromList({"None", "Odd", "Even", "Mark", "Space"}, oldCfg.parity, QString::number(oldCfg.parity)),
              textFromList({"None", "Odd", "Even", "Mark", "Space"}, newCfg.parity, QString::number(newCfg.parity)));
    addChange("流控",
              textFromList({"None", "RTS/CTS", "XON/XOFF", "DTR/DSR"}, oldCfg.flowControl, QString::number(oldCfg.flowControl)),
              textFromList({"None", "RTS/CTS", "XON/XOFF", "DTR/DSR"}, newCfg.flowControl, QString::number(newCfg.flowControl)));
    addChange("接口类型",
              textFromList({"RS-232", "RS-422", "RS-485-2", "RS-485-4"}, oldCfg.interface, QString::number(oldCfg.interface)),
              textFromList({"RS-232", "RS-422", "RS-485-2", "RS-485-4"}, newCfg.interface, QString::number(newCfg.interface)));

    return changes;
}

QStringList MainWindow::lockPortNamesFromPayload(const QByteArray &payload) const
{
    QStringList portNames;
    if (payload.isEmpty())
    {
        return portNames;
    }

    const int count = static_cast<quint8>(payload.at(0));
    for (int i = 0; i < count && i + 1 < payload.size(); ++i)
    {
        portNames << QString("端口%1").arg(static_cast<quint8>(payload.at(i + 1)));
    }

    return portNames;
}

void MainWindow::initTables()
{ 
    /* 1. 搜索设备表 ui->tableWidgetSearch */
    ui->tableWidgetSearch->setColumnCount(9);

    ui->tableWidgetSearch->setHorizontalHeaderLabels({
                                                         " ", "状态", "设备型号", "序列号", "名称", "固件版本",
                                                         "设备状态", "IP地址", "MAC地址"
                                                     });

    ui->allPortShowTableWidget->setColumnCount(8);  // 创建8列
    ui->allPortShowTableWidget->setHorizontalHeaderLabels({"串口号", "串口别名", "波特率", "数据位", "停止位", "校验位", "接口类型", "流控"});

    ui->tableWidget_7->setColumnCount(5);
    ui->tableWidget_7->setHorizontalHeaderLabels({"串口号", "串口别名", "工作模式", "TCP存活检测时间", "最大连接数"});

    /* 2. 端口连接信息 ui->tableWidget_2 */
    ui->tableWidget_2->setColumnCount(4);
    ui->tableWidget_2->setHorizontalHeaderLabels({"串口", "锁定状态", "串口模式", "连接 IP"});

    /* 3. 端口状态计数 ui->tableWidget_3 */
    ui->tableWidget_3->setColumnCount(5);
    ui->tableWidget_3->setHorizontalHeaderLabels({"串口", "TxCnt", "RxCnt", "TxTotal", "RxTotal"});

    /* 4. 端口错误计数 ui->tableWidget_5 */
    ui->tableWidget_5->setColumnCount(6);
    ui->tableWidget_5->setHorizontalHeaderLabels(
    {"串口", "帧错误", "奇偶校验错误", "缓冲区溢出", "Break事件", "总计"});

    /* 5. 端口锁定表 ui->tableWidget_4 */
    /* tableWidget_4：端口锁定表，加复选框 */
    ui->tableWidget_4->setColumnCount(4);
    ui->tableWidget_4->setHorizontalHeaderLabels({"选择", "端口", "锁定状态", "锁定的MAC地址"});
    /* 一次性生成 16 行，每行左侧放 QCheckBox */
    for (int i = 0; i < 16; ++i)
    {
        ui->tableWidget_4->insertRow(i);
        // 第 0 列：复选框
        QCheckBox *cb = new QCheckBox();
        cb->setObjectName(QString("lockCb_%1").arg(i + 1)); // lockCb_1 ~ lockCb_16
        QWidget *w = new QWidget();
        QHBoxLayout *ly = new QHBoxLayout(w);
        ly->addWidget(cb);
        ly->setAlignment(Qt::AlignCenter);
        ly->setContentsMargins(0, 0, 0, 0);
        ui->tableWidget_4->setCellWidget(i, 0, w);

        // 其余列先留空，后续由数据模型填充
        ui->tableWidget_4->setItem(i, 1, new QTableWidgetItem(QString::number(i + 1)));
        ui->tableWidget_4->setItem(i, 2, new QTableWidgetItem("未锁定"));
        ui->tableWidget_4->setItem(i, 3, new QTableWidgetItem(""));
    }


    /* 6. 端口模式/同步列表 ui->tableWidget_6 */
    ui->tableWidget_6->setColumnCount(5);
    ui->tableWidget_6->setHorizontalHeaderLabels({"选择", "端口号", "工作模式", "IP1地址", "IP2地址"});

    // 给主界面按钮添加图表
    QIcon icon;
    // 正常状态
    icon.addPixmap(QPixmap(":/image/deviceSearch(2).png"), QIcon::Normal, QIcon::Off);
    // 按钮按下
    icon.addPixmap(QPixmap(":/image/deviceSearch(1).png"), QIcon::Active, QIcon::Off);
    ui->Search_Btn->setIcon(icon);
    ui->Search_Btn->setIconSize(QSize(25,25));

    // 正常状态
    icon.addPixmap(QPixmap(":/image/delete.png"), QIcon::Normal, QIcon::Off);
    // 按钮按下
    icon.addPixmap(QPixmap(":/image/deleteClick.png"), QIcon::Active, QIcon::Off);
    ui->pushButton_2->setIcon(icon);
    ui->pushButton_2->setIconSize(QSize(25, 25));

    // 正常状态
    icon.addPixmap(QPixmap(":/image/login.png"), QIcon::Normal, QIcon::Off);
    // 按钮按下
    icon.addPixmap(QPixmap(":/image/loginClick.png"), QIcon::Active, QIcon::Off);
    ui->llogin_btn->setIcon(icon);
    ui->llogin_btn->setIconSize(QSize(25, 25));

    // 正常状态
    icon.addPixmap(QPixmap(":/image/driveDetection.png"), QIcon::Normal, QIcon::Off);
    // 按钮按下
    icon.addPixmap(QPixmap(":/image/driveDetectionClick.png"), QIcon::Active, QIcon::Off);
    ui->pushButton_4->setIcon(icon);
    ui->pushButton_4->setIconSize(QSize(25, 25));

    // 正常状态
    icon.addPixmap(QPixmap(":/image/mapping.png"), QIcon::Normal, QIcon::Off);
    // 按钮按下
    icon.addPixmap(QPixmap(":/image/mappingClick.png"), QIcon::Active, QIcon::Off);
    ui->pushButton_5->setIcon(icon);
    ui->pushButton_5->setIconSize(QSize(20, 20));

    // 正常状态
    icon.addPixmap(QPixmap(":/image/disconnect.png"), QIcon::Normal, QIcon::Off);
    // 按钮按下
    icon.addPixmap(QPixmap(":/image/disconnectClick.png"), QIcon::Active, QIcon::Off);
    ui->pushButton_7->setIcon(icon);
    ui->pushButton_7->setIconSize(QSize(25, 25));


    /* -------------------------- 给登陆后的界面按钮添加图标 ---------------------------------- */
    icon.addPixmap(QPixmap(":/image/basicSettings.png"), QIcon::Normal, QIcon::Off);
    icon.addPixmap(QPixmap(":/image/basicSettingsClick.png"), QIcon::Active, QIcon::Off);
    ui->basic_settings->setIcon(icon);
    ui->basic_settings->setIconSize(QSize(25, 25));

    icon.addPixmap(QPixmap(":/image/systemManage.png"), QIcon::Normal, QIcon::Off);
    icon.addPixmap(QPixmap(":/image/systemManageClick.png"), QIcon::Active, QIcon::Off);
    ui->device_manage_btn->setIcon(icon);
    ui->device_manage_btn->setIconSize(QSize(25, 25));

    icon.addPixmap(QPixmap(":/image/reboot.png"), QIcon::Normal, QIcon::Off);
    icon.addPixmap(QPixmap(":/image/rebootClick.png"), QIcon::Active, QIcon::Off);
    ui->restart_btn->setIcon(icon);
    ui->restart_btn->setIconSize(QSize(22, 22));

    icon.addPixmap(QPixmap(":/image/deviceOverview.png"), QIcon::Normal, QIcon::Off);
    icon.addPixmap(QPixmap(":/image/deviceOverviewClick.png"), QIcon::Active, QIcon::Off);
    ui->overview_btn->setIcon(icon);
    ui->overview_btn->setIconSize(QSize(25, 25));

    icon.addPixmap(QPixmap(":/image/settingsPort.png"), QIcon::Normal, QIcon::Off);
    icon.addPixmap(QPixmap(":/image/settingsPortClick.png"), QIcon::Active, QIcon::Off);
    ui->pushButton_10->setIcon(icon);
    ui->pushButton_10->setIconSize(QSize(25, 25));

    icon.addPixmap(QPixmap(":/image/back.png"), QIcon::Normal, QIcon::Off);
    icon.addPixmap(QPixmap(":/image/backClick.png"), QIcon::Active, QIcon::Off);
    ui->pushButton->setIcon(icon);
    ui->pushButton->setIconSize(QSize(25, 25));

    icon.addPixmap(QPixmap(":/image/portManage.png"), QIcon::Normal, QIcon::Off);
    icon.addPixmap(QPixmap(":/image/portManageClick.png"), QIcon::Active, QIcon::Off);
    ui->pushButton_9->setIcon(icon);
    ui->pushButton_9->setIconSize(QSize(25, 25));

    /*----------------------- 设置所有QTableWidget单元格居中显示，且表头自适应文字长度 ------------------------*/
    QList<QTableWidget *>tableWidgets = this->findChildren<QTableWidget *>();
    // 创建并设置自定义委托（实现单元格居中对齐）
    CenterAlignedDelegate *delegate = new CenterAlignedDelegate(this);
    // 在第0列显示复选框
    for(QTableWidget *tableWidget : tableWidgets)
    {
        tableWidget->horizontalHeader()->setStretchLastSection(true);     // 让最后一列的单元格自动拉伸，填满表格的剩余宽度
        tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
        tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows); // 设置选中其中一个单元格会选中该单元格的一整行
        tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);  // 设置禁止修改单元格内容
        tableWidget->verticalHeader()->setVisible(false); // 关闭行号区域
        tableWidget->setItemDelegate(delegate);
        tableWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }
    ui->tableWidgetSearch->setItemDelegateForColumn(0, new CheckBoxCenterDelegate(ui->tableWidgetSearch, QSize(14, 14)));
}

void MainWindow::initTitleBar()
{
    // 1. 无边框窗口设置
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowSystemMenuHint);
    // 去掉 1 px 白边
    setObjectName("MainWindow");

    // 2. 保持主窗口背景设置，使用UI中定义的背景图片
    // 优化重绘，避免拖动闪烁/残留
    setAttribute(Qt::WA_NoSystemBackground, false);
    setAttribute(Qt::WA_DeleteOnClose, true); // 关闭时销毁窗口

    // 3. 添加自定义标题栏到现有的中心控件
    CustomTitleBar *titleBar = new CustomTitleBar(ui->centralwidget);
    
    // 4. 获取现有的布局
    QGridLayout *existingLayout = qobject_cast<QGridLayout*>(ui->centralwidget->layout());
    if (existingLayout)
    {
        // 调整布局，为标题栏腾出空间
        existingLayout->setRowStretch(0, 0);
        existingLayout->setRowStretch(1, 1);
        
        // 移动现有的stackedWidget到第二行
        QWidget *stackedWidget = ui->stackedWidget;
        existingLayout->removeWidget(stackedWidget);
        existingLayout->addWidget(titleBar, 0, 0, 1, existingLayout->columnCount());
        existingLayout->addWidget(stackedWidget, 1, 0, 1, existingLayout->columnCount());
    }
}

void MainWindow::bindDataToUI()
{
    // 安全检查：确保 UI 已初始化
    if (!ui)
    {
        Logger::instance()->log(Logger::Warning, "MainWindow", "UI not initialized!");
        return;
    }

    // 安全检查：确保控制器存在
    if (!m_devCtl)
    {
        Logger::instance()->log(Logger::Warning, "MainWindow", "Device controller not set!");
        return;
    }

    /* 0. 拿到数据模型 */
    DeviceModel *m = m_devCtl->currentDevice();
    if (!m)
    {
        Logger::instance()->log(Logger::Warning, "MainWindow", "No current device model!");
        return;
    }

    // 使用 weak 指针或简单的 this 检查
    Logger::instance()->log(Logger::Debug, "MainWindow", QString("sender: %1, receiver: %2").arg((quintptr)m_devCtl).arg((quintptr)this));

    /* 5. 端口连接信息 → tableWidget_2 自动填充 */
    connect(m_devCtl, &DeviceController::portConnInfoReady, this, [this](const QList<PortConnInfo> &list) {
        if (!ui) return;

        if (ui->tableWidget_2) {
            ui->tableWidget_2->setRowCount(list.size());
            for (int i = 0; i < list.size(); ++i) {
                const auto &v = list.at(i);
                ui->tableWidget_2->setItem(i, 0, new QTableWidgetItem(QString::number(v.portIndex)));
                ui->tableWidget_2->setItem(i, 1, new QTableWidgetItem(v.locked ? "锁定" : "未锁定"));
                ui->tableWidget_2->setItem(i, 2, new QTableWidgetItem(v.modeText));
                ui->tableWidget_2->setItem(i, 3, new QTableWidgetItem(v.connIPs.join("\n")));
            }
        }
    });

    /* 6. 端口状态计数 → tableWidget_3 自动刷新 */
    connect(m_devCtl, &DeviceController::portStatusReady, this, [this](const QList<PortStatusInfo> &list) {
        if (!ui) return;

        if (ui->tableWidget_3) {
            ui->tableWidget_3->setRowCount(list.size());
            for (int i = 0; i < list.size(); ++i) {
                const auto &v = list.at(i);
                ui->tableWidget_3->setItem(i, 0, new QTableWidgetItem(QString::number(v.portIndex)));
                ui->tableWidget_3->setItem(i, 1, new QTableWidgetItem(QString::number(v.txCount)));
                ui->tableWidget_3->setItem(i, 2, new QTableWidgetItem(QString::number(v.rxCount)));
                ui->tableWidget_3->setItem(i, 3, new QTableWidgetItem(QString::number(v.txTotalCount)));
                ui->tableWidget_3->setItem(i, 4, new QTableWidgetItem(QString::number(v.rxTotalCount)));
            }
        }
    });

    /* 7. 端口错误计数 → tableWidget_5 自动刷新 */
    connect(m_devCtl, &DeviceController::portErrorReady, this, [this](const QList<PortErrorInfo> &list) {
        if (!ui) return;

        if (ui->tableWidget_5)
        {
            ui->tableWidget_5->setRowCount(list.size());
            for (int i = 0; i < list.size(); ++i)
            {
                const auto &v = list.at(i);
                ui->tableWidget_5->setItem(i, 0, new QTableWidgetItem(QString::number(v.portIndex)));
                ui->tableWidget_5->setItem(i, 1, new QTableWidgetItem(QString::number(v.frameErr)));
                ui->tableWidget_5->setItem(i, 2, new QTableWidgetItem(QString::number(v.parityErr)));
                ui->tableWidget_5->setItem(i, 3, new QTableWidgetItem(QString::number(v.overrunErr)));
                ui->tableWidget_5->setItem(i, 4, new QTableWidgetItem(QString::number(v.breakErr)));
                quint64 total = v.frameErr + v.parityErr + v.overrunErr + v.breakErr;
                ui->tableWidget_5->setItem(i, 5, new QTableWidgetItem(QString::number(total)));
            }
        }
    });


    // 程序第一次运行时执行updateLan1Gray(存储一个lamda表达式)
    //    auto updateLan1Gray = [this](const QString &text) {
    //        bool isStatic = (text != "DHCP" && text != "无");
    //        ui->lan1_ip      ->setEnabled(isStatic);
    //        ui->lan1_mask    ->setEnabled(isStatic);
    //        ui->lan1_gateway ->setEnabled(isStatic);
    //        ui->label_6      ->setEnabled(isStatic);   // 标签也灰
    //        ui->label_7      ->setEnabled(isStatic);
    //        ui->label_8      ->setEnabled(isStatic);
    //    };
    //    connect(ui->lan1_comboBox, &QComboBox::currentTextChanged, this, updateLan1Gray);
    //    updateLan1Gray(ui->lan1_comboBox->currentText()); // 初始状态

    connect(m_devCtl, &DeviceController::portLockStatusReady,
            this, &MainWindow::onPortLockStatusReady);
}

QList<int> MainWindow::getSelectedRowIndices() const
{
    QList<int> selectedRows;
    for (int row = 0; row < ui->tableWidget_4->rowCount(); ++row)
    {
        QWidget *widget = ui->tableWidget_4->cellWidget(row, 0);
        if (widget)
        {
            QCheckBox *cb = widget->findChild<QCheckBox*>();
            if (cb && cb->isChecked())
            {
                selectedRows.append(row);
            }
        }
    }
    return selectedRows;
}

QByteArray MainWindow::collectSelectedLockPorts(int i) const
{
    QByteArray bits(32, 0);   // 固定 32 字节，默认全 0
    int used = 0;             // 实际写入个数（≤32）
    int used_2 = 0;

    QList<int> selectedRows = getSelectedRowIndices();
    for (int a : selectedRows)
    {
        QTableWidgetItem *item = ui->tableWidget_4->item(a, 2);
        if (i == 1 && item->text() == "未锁定")
        {
            bits[used++] = static_cast<quint8>(a+1);
        }
        if (i == 2 && item->text() == "已锁定")
        {
            bits[used++] = static_cast<quint8>(a+1);
        }
    }
    used_2 = used;
    QByteArray hexString = QByteArray::fromHex((QString("%1").arg(used_2, 2, 16, QChar('0'))).toUtf8()); //个数
    QByteArray result = bits.left(used);        //端口序号     // 只返回实际使用的部分
    hexString.append(result);
    Logger::instance()->log(Logger::Debug, "MainWindow", QString("collectSelectedLockPorts hexString: %1").arg(QString(hexString.toHex(' '))));
    return hexString;   // 格式：2字节个数 + 实际端口号
}

bool MainWindow::validateFirmwarePackage(const QString &filePath, QByteArray &outPkg)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return false;
    outPkg = file.readAll();
    file.close();

    //    if (outPkg.size() < FW_PACKAGE_HEADER_SIZE) return false;

    //    const fw_package_header_t *header = reinterpret_cast<const fw_package_header_t*>(outPkg.constData());

    //    // 1. Magic
    //    if (header->magic_number != FW_PACKAGE_MAGIC) return false;

    //    // 2. Header CRC-32-MPEG2
    //    QByteArray headerData = outPkg.mid(8, FW_PACKAGE_HEADER_SIZE - 8);
    //    quint32 headerCReal = crc32mpeg(headerData);
    //    if (headerCReal != header->header_crc32) return false;

    //    // 3. 总长度
    //    quint32 expected = FW_PACKAGE_HEADER_SIZE + header->bit_length + header->app_length;
    //    if (outPkg.size() != expected) return false;

    //    // 4. Bitstream CRC-32-MPEG2
    //    QByteArray bitData = outPkg.mid(FW_PACKAGE_HEADER_SIZE, header->bit_length);
    //    quint32 bitCReal = crc32mpeg(bitData);
    //    if (bitCReal != header->bit_crc32) return false;

    Logger::instance()->log(Logger::Debug, "MainWindow", "[PKG] 预校验通过");
    return true;
}


void MainWindow::on_pushButton_7_clicked()
{
    bool a = m_devCtl->deviceDisconnected();
    if(a == false)
    {
        QMessageBox::information(this, "设备断开", "请先连接设备!!!");
        appendDeviceLog("操作失败：请先连接设备");
    }
    else
    {
        QMessageBox::information(this, "设备断开", QString("成功断开与%1的连接").arg(UdpDiscovery::dev.lan1Ip));
        appendDeviceLog(QString("断开设备：%1").arg(currentCheckedDeviceText()));
        TcpClient::TcpSocketState = false;

        // 清空tableWidgetSearch的高亮状态以及焦点
        ui->tableWidgetSearch->clearSelection();
        ui->tableWidgetSearch->setCurrentCell(-1, -1);

        // 断开连接后清空文本控件的内容
        QList<QTextBrowser *> TBList = ui->stackedWidget->findChildren<QTextBrowser *>();
        for (QTextBrowser* textBrowser : TBList)
        {
            textBrowser->clear();
        }
        QList<QLineEdit *> LEList = ui->stackedWidget->findChildren<QLineEdit *>();
        for (QLineEdit* lineEdit : LEList)
        {
            lineEdit->clear();
        }
        QList<QTableWidget *> TWList = ui->stackedWidget->findChildren<QTableWidget *>();
        for (QTableWidget* tableWidget : TWList)
        {
            if (tableWidget->objectName() != "tableWidgetSearch")
            {
                tableWidget->setRowCount(0);
            }
        }
    }
}


void MainWindow::on_tabWidget_currentChanged(int index)
{
    switch (index)
    {
    case 0:
        ui->comboBox_6->setCurrentIndex(0);
        ui->work_mode_comboBox->setCurrentIndex(0);
        ui->stackedWidget_3->setCurrentIndex(1);
        ui->alias_lineEdit->setEnabled(true);
        ui->PortNewNameCBox->setChecked(false);
    {
        QList<QCheckBox*> checkBoxes = this->findChildren<QCheckBox*>(QRegularExpression("checkBox_(1[7-9]|2[0-9]|3[0-2])"));
        for(QCheckBox *checkBox : checkBoxes)
        {
            checkBox->setChecked(false);
        }
    }
        ui->checkBox_17->setChecked(true);

        m_devCtl->readSerialConfig(ui->comboBox_6->currentIndex() + 1);
        break;

    case 1:
        ui->comboBox_7->setCurrentIndex(0);
        ui->work_mode_comboBox->setCurrentIndex(0);
        ui->stackedWidget_3->setCurrentIndex(1);
    {
        QList<QCheckBox*> checkBoxes = ui->tab_2->findChildren<QCheckBox*>();
        for(QCheckBox *checkBox : checkBoxes)
        {
            checkBox->setChecked(false);
        }
    }
        ui->checkBox_35->setChecked(true);

        m_devCtl->queryPortMode(ui->comboBox_7->currentIndex() + 1);
        break;
    }
}

void MainWindow::on_work_mode_comboBox_activated(const QString &arg1)
{
    Logger::instance()->log(Logger::Debug, "MainWindow", QString("arg1: %1").arg(arg1));
    if(arg1 == "Real COM Mode ")
    {
        ui->stackedWidget_3->setCurrentIndex(1);
    }
    else if(arg1 == "TCP Server Mode")
    {
        ui->stackedWidget_3->setCurrentIndex(0);
    }
    else if(arg1 == "Redundant Mode")
    {
        ui->stackedWidget_3->setCurrentIndex(2);
    }
}

void MainWindow::on_alias_lineEdit_textChanged(const QString &arg1)
{
    if(arg1.length() > 19)
    {
        QMessageBox::StandardButton reply = QMessageBox::warning(this, "警告", "别名最大长度为19字符", QMessageBox::Yes);
        if(reply == QMessageBox::Yes)
        {
            ui->alias_lineEdit->clear();
        }
    }
}

void MainWindow::on_comboBox_7_activated(int index)
{

    QList<QCheckBox*> checkBoxes = this->findChildren<QCheckBox*>(QRegularExpression("checkBox_(3[5-9]|4[0-9]|5[0])"));

    for(QCheckBox *checkBox : checkBoxes)
    {
        checkBox->setChecked(false);
    }

    QString targetName = QString("checkBox_%1").arg(index + 35);
    QCheckBox *targetCheckBox = ui->tab_2->findChild<QCheckBox *>(targetName);
    if(targetCheckBox != nullptr)
    {
        targetCheckBox->setChecked(true);
    }
    else
    {
        Logger::instance()->log(Logger::Debug, "MainWindow", "没有此控件!!");
    }
    m_devCtl->queryPortMode(index + 1);

    return;
}

void MainWindow::on_comboBox_6_activated(int index)
{
    QList<QCheckBox*> checkBoxes = this->findChildren<QCheckBox*>(QRegularExpression("checkBox_(1[7-9]|2[0-9]|3[0-2])"));

    for(QCheckBox *checkBox : checkBoxes)
    {
        checkBox->setChecked(false);
    }
    QString targetName = QString("checkBox_%1").arg(index + 17);
    QCheckBox *targetCheckBox = this->findChild<QCheckBox *>(targetName);
    if(targetCheckBox != nullptr)
    {
        targetCheckBox->setChecked(true);
    }
    else
    {
        Logger::instance()->log(Logger::Debug, "MainWindow", "没有此控件!!");
    }
    m_devCtl->readSerialConfig(index + 1);
    return ;
}

void MainWindow::on_new_name_cursorPositionChanged(int arg1, int arg2)
{
    if(arg2 == 0)
    {
        // 设置文本颜色和字体大小
        ui->new_name->setStyleSheet("QLineEdit#new_name{"
                                    " color: rgba(168,200,236, 0.3); "
                                    "font-family: 'Microsoft YaHei'; "
                                    "font-size: 12px; "
                                    "}");
    }
    if((arg1 > 0 && arg2 != 0) || (arg1 == 0 && arg2 == 1) || arg2 > 0)
    {
        ui->new_name->setStyleSheet("QLineEdit#new_name{"
                                    "color: white; "
                                    "}");
    }

}

void MainWindow::on_alias_lineEdit_cursorPositionChanged(int arg1, int arg2)
{
    if(arg2 == 0 && ui->alias_lineEdit->text().isEmpty())
    {
        // 设置文本颜色和字体大小
        ui->alias_lineEdit->setStyleSheet("QLineEdit#alias_lineEdit{"
                                          " color: rgba(168,200,236, 0.3); "
                                          "font-family: 'Microsoft YaHei'; "
                                          "font-size: 12px; "
                                          "}");
    }
    if((arg1 > 0 && arg2 != 0) || (arg1 == 0 && arg2 == 1) || arg2 > 0)
    {
        ui->alias_lineEdit->setStyleSheet("QLineEdit#alias_lineEdit{"
                                          "color: white; "
                                          "}");
    }
}


void MainWindow::on_chooseAllPortCheckBox_stateChanged(int arg1)
{
    if (arg1 == Qt::Checked)
    {
        m_devCtl->readSerialConfig(1616);
    }
}

void MainWindow::on_PortNewNameCBox_stateChanged(int arg1)
{
    if (arg1 == Qt::Checked)
    {
        ui->alias_lineEdit->setEnabled(false);
    }
    if (arg1 == Qt::Unchecked)
    {
        ui->alias_lineEdit->setEnabled(true);
    }
}

/*
 * 处理表格项变化事件
 * @param item 发生变化的表格项
 * 功能：确保表格中只能有一个复选框被选中（单选功能）
 */
void MainWindow::onItemChanged(QTableWidgetItem *item)
{
    // 只处理第0列（复选框列）的变化
    if (item->column() != 0)
    {
        return;
    }
    
    // 只处理可勾选的项目
    if (!(item->flags() & Qt::ItemIsUserCheckable))
    {
        return;
    }
    
    // 当项目被勾选时
    if (item->checkState() == Qt::Checked)
    {
        // 遍历所有行
        for (int row = 0; row < ui->tableWidgetSearch->rowCount(); ++row)
        {
            // 跳过当前行
            if (row != item->row())
            {
                // 获取其他行的复选框
                QTableWidgetItem *otherItem = ui->tableWidgetSearch->item(row, 0);
                // 如果其他行有被勾选的复选框，取消其勾选状态
                if (otherItem && otherItem->checkState() == Qt::Checked)
                {
                    otherItem->setCheckState(Qt::Unchecked);
                }
            }
        }
    }
}
