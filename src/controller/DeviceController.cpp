#include "DeviceController.h"
#include "../utils/ProtocolConstants.h"
#include <QtEndian>
#include <QCoreApplication>
#include <QThread>

DeviceController::DeviceController(QObject *parent)
    : QObject(parent)
    , m_currentDevice(new DeviceModel(this))
    , m_tcpClient(new TcpClient(this))
    , m_udpDiscovery(UdpDiscovery::instance(this))
    , m_connManager(new ConnectionManager(m_tcpClient, this))
    , m_parser(new ProtocolParser(this))
    , m_logger(Logger::instance())
    , m_pendingOperation(PendingOperation::None)
{
    // 连接TCP客户端信号
    connect(m_tcpClient, &TcpClient::connected, this, &DeviceController::onTcpConnected);
    connect(m_tcpClient, &TcpClient::disconnected, this, &DeviceController::onTcpDisconnected);
    connect(m_tcpClient, &TcpClient::errorOccurred, this, &DeviceController::onTcpError);
    connect(m_tcpClient, &TcpClient::frameReceived, this, &DeviceController::onFrameReceived);

    // 当固件升级文件传输完毕。若传输错误，设备会返回ERROR 0x00, 成功无返回
    connect(m_tcpClient, &TcpClient::updateBufferHaveData, this, [=](bool ifOK){
        if(ifOK == false)
        {
            m_logger->log(Logger::Error, "DeviceController", "Device Update Error");
            return ;
        }
        m_logger->log(Logger::Info, "DeviceController", "Device Update success");
    });
    // 连接UDP发现信号
    connect(m_udpDiscovery, &UdpDiscovery::deviceDiscovered, this, &DeviceController::onDeviceDiscovered);
    connect(m_udpDiscovery, &UdpDiscovery::searchCompleted, this, &DeviceController::onSearchCompleted);
    connect(m_udpDiscovery, &UdpDiscovery::searchError, this, &DeviceController::onSearchError);
    connect(m_udpDiscovery, &UdpDiscovery::bindSucceeded, this, &DeviceController::udpBindSucceeded);

    // 连接ConnectionManager信号
    connect(m_connManager, &ConnectionManager::commandTimeout, this, &DeviceController::onCommandTimeout);

    m_logger->log(Logger::Info, "DeviceController", "Device Controller initialized");
}

DeviceController::~DeviceController()
{
    disconnectFromDevice();
    m_logger->log(Logger::Info, "DeviceController", "Device Controller destroyed");
}

void DeviceController::searchDevices()
{
    m_logger->log(Logger::Info, "DeviceController", "Starting device search");
    m_udpDiscovery->startSearch();
}

void DeviceController::connectToDevice(const QString &ip, quint16 port)
{
    m_logger->log(Logger::Info, "DeviceController",
                  QString("Connecting to device at %1:%2").arg(ip).arg(port));

    // 设置连接信息用于自动重连
    m_connManager->setConnectionInfo(ip, port);

    // 连接到设备
    m_tcpClient->connectToServer(ip, port);
}

void DeviceController::disconnectFromDevice()
{
    m_logger->log(Logger::Debug, "DeviceController", "tcp2");
    if (m_tcpClient->isUpdateConnected())
    {
        m_logger->log(Logger::Info, "DeviceController", "Disconnecting from device update");
        m_tcpClient->disconnectFromUpdateServer();
    }
}

void DeviceController::readOverview()
{
    m_logger->log(Logger::Info, "DeviceController", "Reading device overview");
    m_pendingOperation = PendingOperation::ReadOverview;
    QByteArray cmd = ProtocolBuilder::buildOverviewQuery();
    m_pendingOperationName = "Read Overview";
    logSend(cmd);
    m_connManager->sendCommand(cmd);
}

// 构建读取设备信息命令
void DeviceController::readNetworkConfig()
{
    m_logger->log(Logger::Info, "DeviceController", "Reading network config");
    m_pendingOperation = PendingOperation::ReadNetworkConfig;
    QByteArray cmd = ProtocolBuilder::buildNetworkConfigQuery();
    m_pendingOperationName = "Read Network Config";
    logSend(cmd);
    m_connManager->sendCommand(cmd);
}

void DeviceController::readSerialConfig(int portIndex)
{
    m_logger->log(Logger::Info, "DeviceController",
                  QString("Reading serial config for port %1").arg(portIndex));
    m_pendingOperation = PendingOperation::ReadSerialConfig;
    m_pendingOperationName = "Read Serial Config";

    QByteArray cmd = ProtocolBuilder::buildSerialConfigQuery(portIndex);
    logSend(cmd);
    m_connManager->sendCommand(cmd);
}

void DeviceController::readPortLockStatus()
{
    m_logger->log(Logger::Info, "DeviceController", "Reading port lock status");
    m_pendingOperation = PendingOperation::ReadPortLockStatus;
    m_pendingOperationName = "Read Port Lock Status";

    QByteArray cmd = ProtocolBuilder::buildPortLockStatusQuery();
    logSend(cmd);
    m_connManager->sendCommand(cmd);
}

void DeviceController::writeDeviceName(const QString &name)
{
    m_logger->log(Logger::Info, "DeviceController",
                  QString("Writing device name: %1").arg(name));
    m_pendingOperation = PendingOperation::WriteDeviceName;
    QByteArray cmd = ProtocolBuilder::buildDeviceNameWrite(name);
    logSend(cmd);
    m_connManager->sendCommand(cmd);
}

void DeviceController::writeNetworkConfig(const NetworkConfig &config)
{
    m_logger->log(Logger::Info, "DeviceController", "Writing network config");
    m_pendingOperation = PendingOperation::WriteNetworkConfig;
    QByteArray cmd = ProtocolBuilder::buildNetworkConfigWrite(config);
    logSend(cmd);
    m_connManager->sendCommand(cmd);
}

void DeviceController::writeSerialConfig(const SerialPortConfig &config, int i)
{
    m_logger->log(Logger::Info, "DeviceController",
                  QString("Writing serial config for port %1").arg(config.index));
    m_pendingOperation = PendingOperation::WriteSerialConfig;
    m_pendingOperationName = "Write Serial Config";
    if (i == 0)     //设置多个端口
    {
        QByteArray cmd = ProtocolBuilder::buildSerialConfigWrite1(config);
        logSend(cmd);
        m_connManager->sendCommand(cmd);
    }
    else
    {
        QByteArray cmd = ProtocolBuilder::buildSerialConfigWrite(config);
        logSend(cmd);
        m_connManager->sendCommand(cmd);
    }
}

void DeviceController::writeMultiSerialConfig(const QList<SerialPortConfig> &configs)
{
    m_logger->log(Logger::Info, "DeviceController",
                  QString("Writing %1 serial configs").arg(configs.size()));
    m_pendingOperation = PendingOperation::WriteMultiSerialConfig;
    m_pendingOperationName = "Write Multi Serial Config";

    QByteArray cmd = ProtocolBuilder::buildMultiSerialConfigWrite(configs);
    logSend(cmd);
    m_connManager->sendCommand(cmd);
}

void DeviceController::lockPort(int portIndex)
{
    m_logger->log(Logger::Info, "DeviceController",
                  QString("Locking port %1").arg(portIndex));
    m_pendingOperation = PendingOperation::LockPort;
    m_pendingOperationName = "Lock Port";

    QByteArray cmd = ProtocolBuilder::buildPortLock(portIndex);
    logSend(cmd);
    m_connManager->sendCommand(cmd);
}

void DeviceController::unlockPort(int portIndex)
{
    m_logger->log(Logger::Info, "DeviceController",
                  QString("Unlocking port %1").arg(portIndex));
    m_pendingOperation = PendingOperation::UnlockPort;
    m_pendingOperationName = "Unlock Port";

    QByteArray cmd = ProtocolBuilder::buildPortUnlock(portIndex);
    logSend(cmd);
    m_connManager->sendCommand(cmd);
}

void DeviceController::lockDevice()
{
    m_logger->log(Logger::Info, "DeviceController", "Locking device");
    m_pendingOperation = PendingOperation::LockDevice;
    m_pendingOperationName = "Lock/UnLock Device";
    QByteArray cmd = ProtocolBuilder::buildDeviceLock();
    logSend(cmd);
    m_connManager->sendCommand(cmd);
}

void DeviceController::unlockDevice(const QString &code)
{
    m_logger->log(Logger::Info, "DeviceController", "Unlocking device");
    m_pendingOperation = PendingOperation::UnlockDevice;
    m_pendingOperationName = "Lock/UnLock Device";
    QByteArray cmd = ProtocolBuilder::buildDeviceUnlock(code);
    logSend(cmd);
    m_connManager->sendCommand(cmd);
}

void DeviceController::factoryReset()
{
    m_logger->log(Logger::Info, "DeviceController", "Factory reset");
    m_pendingOperation = PendingOperation::FactoryReset;
    m_pendingOperationName = "Factory Reset";

    QByteArray cmd = ProtocolBuilder::buildFactoryReset();
    logSend(cmd);
    m_connManager->sendCommand(cmd);
}

void DeviceController::reboot()
{
    m_logger->log(Logger::Info, "DeviceController", "Rebooting device");
    m_pendingOperation = PendingOperation::Reboot;
    m_pendingOperationName = "Reboot";

    QByteArray cmd = ProtocolBuilder::buildReboot();
    logSend(cmd);
    m_connManager->sendCommand(cmd);
}

DeviceModel* DeviceController::currentDevice() const
{
    return m_currentDevice;
}

void DeviceController::selfCheckPort(quint8 portByte)
{
    m_pendingOperation = PendingOperation::SelfCheck;
    m_pendingOperationName = "Self Check";
    QByteArray cmd = ProtocolBuilder::buildSelfCheck(portByte);
    logSend(cmd);
    m_connManager->sendCommand(cmd);
}

void DeviceController::queryPortMode(int portIndex)
{
    m_logger->log(Logger::Info, "DeviceController", "query port mode");
    m_pendingOperation = PendingOperation::ReadPortMode;
    m_pendingOperationName = "Read Port Mode";

    QByteArray cmd = ProtocolBuilder::buildPortQuery(portIndex);
    logSend(cmd);
    m_connManager->sendCommand(cmd);
}

void DeviceController::queryPortSpecifiedMode(const QByteArray &mode)
{
    m_logger->log(Logger::Info, "DeviceController", "query port specified mode");
    m_pendingOperation = PendingOperation::ReadPortSpecifiedMode;
    m_pendingOperationName = "Read Port Specified Mode";

    QByteArray cmd = ProtocolBuilder::buildPortSpecifiedMode(mode);
    logSend(cmd);
    m_connManager->sendCommand(cmd);
}


void DeviceController::writeModeConfig(int port, quint8 mode, const QVariantMap &params, const SerialPortConfig &config, int i)
{
    m_pendingOperation = PendingOperation::WriteMode;
    m_pendingOperationName = "Write Mode";

    //如果i=0，走启用同步
    if(i == 0)
    {
        QByteArray cmd = ProtocolBuilder::buildModeConfigWrite1(mode, params, config);
        logSend(cmd);
        m_connManager->sendCommand(cmd);
    }
    else
    {
        QByteArray cmd = ProtocolBuilder::buildModeConfigWrite(port, mode, params);
        logSend(cmd);
        m_connManager->sendCommand(cmd);
    }
}

/* ============================================================
 * 端口镜像设置  功能ID=0x08 子功能ID=0x05
 * ============================================================ */
void DeviceController::setPortMirror(const QByteArray &pkt)
{
    m_pendingOperation = PendingOperation::WriteMirror;
    m_pendingOperationName = "Port Mirror";
    QByteArray cmd = ProtocolBuilder::buildDeviceManageCommand(0x05, pkt);
    logSend(cmd);
    m_connManager->sendCommand(cmd);
}

/* ============================================================
 * 看门狗配置    功能ID=0x08 子功能ID=0x09
 * ============================================================ */
void DeviceController::setWatchDog(const QByteArray &pkt)
{
    m_pendingOperation = PendingOperation::WriteWatchDog;
    m_pendingOperationName = "WatchDog";
    QByteArray cmd = ProtocolBuilder::buildDeviceManageCommand(0x09, pkt);
    logSend(cmd);
    m_connManager->sendCommand(cmd);
}

/* ============================================================
 * 旧流固件升级（分包传输）功能ID=0x0A
 * ============================================================ */
void DeviceController::startFirmwareUpgrade(const QString &filePath)
{
    if (m_tcpClient->isConnected())
        emit operationFailed("FirmwareUpgrade", "请先断开当前连接再升级");
    // 实际升级流程由 MainWindow 调用 TcpClient 完成，这里仅做状态标记
    m_pendingOperation = PendingOperation::FirmwareUpgrade;
    m_pendingOperationName = "FirmwareUpgrade";
    // 通知界面开始升级（由 UI 层调用 TcpClient 发送握手帧）
    emit upgradeHandshakeRequested(filePath);
}

/* 0x07 0x01 读取串口网络连接信息（link_info） */
void DeviceController::readPortConnInfo()
{
    m_pendingOperation = PendingOperation::ReadPortConnInfo;
    m_pendingOperationName = "Read Port Conn Info";
    QByteArray cmd = ProtocolBuilder::buildStatusCommand(0x01, QByteArray(2, 0xFF)); // 端口位图 0xFFFF
    logSend(cmd);
    m_connManager->sendCommand(cmd);
}

/* 0x06 0x02 读取端口状态计数（Status_information） */
void DeviceController::readPortStatus()
{
    m_pendingOperation = PendingOperation::ReadPortStatus;
    m_pendingOperationName = "Read Port Status";
    QByteArray cmd = ProtocolBuilder::buildStatusCommand(0x02, QByteArray(2, 0xFF));
    logSend(cmd);
    m_connManager->sendCommand(cmd);
}

/* 0x06 0x03 读取端口错误计数（Error_Message） */
void DeviceController::readPortError()
{
    m_pendingOperation = PendingOperation::ReadPortError;
    m_pendingOperationName = "Read Port Error";
    QByteArray cmd = ProtocolBuilder::buildStatusCommand(0x03, QByteArray(2, 0xFF));
    logSend(cmd);
    m_connManager->sendCommand(cmd);
}

void DeviceController::lockPortsWithSync(const QByteArray &bitmap)
{
    if (bitmap.size() == 0) return;
    m_pendingOperation = PendingOperation::LockPort;
    m_pendingOperationName = "port lock/unlock";
    QByteArray cmd = ProtocolBuilder::buildMultiPortLock(bitmap, true);  // true=锁定
    logSend(cmd);
    m_connManager->sendCommand(cmd);
}

void DeviceController::unlockPortsWithSync(const QByteArray &bitmap)
{
    if (bitmap.size() == 0) return;
    m_pendingOperation = PendingOperation::UnlockPort;
    m_pendingOperationName = "port lock/unlock";
    QByteArray cmd = ProtocolBuilder::buildMultiPortLock(bitmap, false); // false=解锁
    logSend(cmd);
    m_connManager->sendCommand(cmd);
}

void DeviceController::logSend(const QByteArray &data)
{
    Logger::instance()->log(Logger::Info, "DeviceController", QString("[SEND] %1").arg(QString(data.toHex(' ').toUpper())));
}

// 自检
void DeviceController::appendSyncBitmap(const QByteArray &bitmap)
{
    int i = static_cast<unsigned char>(bitmap.at(0));
    if(i == 0 || i > 16 ) return;
    m_pendingOperation = PendingOperation::SelfCheck;
    m_pendingOperationName = "SelfCheckSync";
    QByteArray cmd = ProtocolBuilder::buildSelfCheckWithSync(bitmap);
    logSend(cmd);
    m_connManager->sendCommand(cmd);
}

// 重置
void DeviceController::buildPortReset(const QByteArray &bitmap)
{
    int i = static_cast<unsigned char>(bitmap.at(0));
    if(i == 0 || i > 16 ) return;
    m_pendingOperation = PendingOperation::ResetPort;
    m_pendingOperationName = "SelfCheckSync";
    QByteArray cmd = ProtocolBuilder::buildSelfCheckWithSync1(bitmap);
    logSend(cmd);
    m_connManager->sendCommand(cmd);
}

void DeviceController::onFrameReceived(const QByteArray &frame)
{
    //显示通用数据帧，功能ID和子功能ID
    m_logger->log(Logger::Info, "DeviceController", QString("Frame received (%1 bytes)").arg(QString(frame.toHex(' ')).toUpper()));

    // 验证帧
    if (!m_parser->validateFrame(frame))
    {
        m_logger->log(Logger::Error, "DeviceController", "Invalid frame received");
        emit operationFailed(m_pendingOperationName, "Invalid frame");
        m_pendingOperation = PendingOperation::None;
        return;
    }

    QByteArray payload;
    payload = frame.mid(4, frame.size() - 6);   //帧头(2B) + 帧尾(2B) + 主ID副ID(2Byte)

    // 根据当前操作处理响应
    switch (m_pendingOperation)
    {
    case PendingOperation::ReadOverview:
        m_pendingOperation = PendingOperation::None;
        handleOverviewResponse(payload);
        break;

    case PendingOperation::ReadNetworkConfig:
        m_pendingOperation = PendingOperation::None;
        handleNetworkConfigResponse(payload);
        break;

    case PendingOperation::ReadSerialConfig:
        m_pendingOperation = PendingOperation::None;
        handleSerialConfigResponse(payload);
        break;

    case PendingOperation::ReadPortLockStatus:
        m_pendingOperation = PendingOperation::None;
        handlePortLockStatusResponse(payload);
        break;

    case PendingOperation::ReadPortConnInfo:
        m_pendingOperation = PendingOperation::None;
        handlePortConnInfoResponse(payload);
        break;

    case PendingOperation::ReadPortStatus:
        m_pendingOperation = PendingOperation::None;
        handlePortStatusResponse(payload);
        break;

    case PendingOperation::ReadPortError:
        m_pendingOperation = PendingOperation::None;
        handlePortErrorResponse(payload);
        break;

    case PendingOperation::LockDevice:
        m_pendingOperation = PendingOperation::None;
        handleDeviceLockResponse(payload);
        break;
    case PendingOperation::UnlockDevice:
        m_pendingOperation = PendingOperation::None;
        handleDeviceUnLockResponse(payload);
        break;
    case PendingOperation::Reboot:
        m_pendingOperation = PendingOperation::None;
        handleWriteResponse(payload);
        break;

    case PendingOperation::WriteDeviceName:
        m_pendingOperation = PendingOperation::None;
        handleDeviceNameReviseResponse(payload);
        break;

    case PendingOperation::WriteNetworkConfig:
        m_pendingOperation = PendingOperation::None;
        handleDeviceConfig(payload);
        break;

    case PendingOperation::LockPort:
        m_pendingOperation = PendingOperation::None;
        handlePortLock(payload);
        break;
    case PendingOperation::UnlockPort:
        m_pendingOperation = PendingOperation::None;
        handlePortUnLock(payload);
        break;
    case PendingOperation::WriteMode:
    case PendingOperation::WriteSerialConfig:
        m_pendingOperation = PendingOperation::None;
        handleWriteSerialConfig(payload);
        break;
    case PendingOperation::FactoryReset:
        m_pendingOperation = PendingOperation::None;
        handleWriteResponse(payload);
        break;
    case PendingOperation::ReadPortMode:
        m_pendingOperation = PendingOperation::None;
        handleReadPortMode(payload);
        break;
    case PendingOperation::SelfCheck:
        m_pendingOperation = PendingOperation::None;
        handleSelfCheck(payload);
        break;
    case PendingOperation::WriteMultiSerialConfig:
    case PendingOperation::ResetPort:

    default:
        m_logger->log(Logger::Warning, "DeviceController", "Unexpected frame received");
        break;
    }
}

void DeviceController::onDeviceDiscovered(const DeviceInfo &device)
{
    m_logger->log(Logger::Info, "DeviceController",
                  QString("Device discovered: %1 at %2")
                  .arg(device.deviceName).arg(device.lan1Ip));
    emit deviceDiscovered(device);
}

void DeviceController::onCommandTimeout()
{
    m_logger->log(Logger::Error, "DeviceController",
                  QString("Command timeout: %1").arg(m_pendingOperationName));
    emit operationFailed(m_pendingOperationName, "Command timeout");
    m_pendingOperation = PendingOperation::None;
}

void DeviceController::onTcpConnected()
{
    m_logger->log(Logger::Info, "DeviceController", "TCP connected");
    emit connected();
}

void DeviceController::onTcpDisconnected()
{
    m_logger->log(Logger::Info, "DeviceController", "TCP disconnected");
    emit disconnected();
}

void DeviceController::onTcpError(const QString &error)
{
    m_logger->log(Logger::Error, "DeviceController",
                  QString("TCP error: %1").arg(error));
    emit connectionError(error);
}

void DeviceController::onSearchCompleted(int count)
{
    m_logger->log(Logger::Info, "DeviceController",
                  QString("Search completed, found %1 device(s)").arg(count));
    emit searchCompleted(count);
}

void DeviceController::onSearchError(const QString &error)
{
    m_logger->log(Logger::Error, "DeviceController",
                  QString("Search error: %1").arg(error));
    emit operationFailed("Device Search", error);
}

void DeviceController::handleOverviewResponse(const QByteArray &payload)
{
    DeviceInfo info;
    if (m_parser->parseOverviewResponse(payload, info))
    {
        // 更新设备模型
        m_currentDevice->setDeviceInfo(info);
        m_logger->log(Logger::Info, "DeviceController", "Overview data updated");
        emit operationSuccess(m_pendingOperationName);
    }
    else
    {
        m_logger->log(Logger::Error, "DeviceController", "Failed to parse overview response");
        emit operationFailed(m_pendingOperationName, "Parse error");
    }
}

void DeviceController::handleNetworkConfigResponse(const QByteArray &payload)
{
    NetworkConfig config;
    if (m_parser->parseNetworkConfigResponse(payload, config))
    {
        // 更新设备模型
        m_currentDevice->setNetworkConfig(config);
        m_logger->log(Logger::Info, "DeviceController", "Network config updated");
        m_pendingOperationName = "Read Network Config";
        emit operationSuccess(m_pendingOperationName);
    }
    else
    {
        m_logger->log(Logger::Error, "DeviceController", "Failed to parse network config response");
        emit operationFailed(m_pendingOperationName, "Parse error");
    }
}

void DeviceController::handleSerialConfigResponse(const QByteArray &payload)
{
    QList<SerialPortConfig> configs;
    if (m_parser->parseSerialConfigResponse(payload, configs))
    {

        m_currentDevice->setSerialPorts(configs);
        m_logger->log(Logger::Info, "DeviceController",
                      QString("Serial config updated (%1 ports)").arg(configs.size()));

        emit serialConfigUpdated(); // 未使用的信号
        emit operationSuccess(m_pendingOperationName);
    }
    else
    {
        m_logger->log(Logger::Error, "DeviceController", "Failed to parse serial config response");
        emit operationFailed(m_pendingOperationName, "Parse error");
    }
}

void DeviceController::handlePortLockStatusResponse(const QByteArray &p)
{
    const int BLOCK = 7;   // 每端口 7 字节： 1 状态 + 6 MAC（ASCII）
    if ((p.mid(1)).size() != BLOCK * 16)
    {
        emit operationFailed(m_pendingOperationName, "数据长度不足");
        return ;
    }
    QByteArray p2 = p.mid(1);
    QList<PortLockInfo> list;
    for (int i = 0; i < 16; ++i)
    {
        int off = i * BLOCK;
        PortLockInfo info;
        info.portNum = (i + 1);
        QByteArray portInformation = p2.mid(off, 7);
        QString state = QString::fromLatin1(portInformation.mid(0, 1).toHex());
        if (state == "01")
        {
            info.lockState = true;
        }
        else if (state == "00")
        {
            info.lockState = false;
        }
        else
        {
            m_logger->log(Logger::Error, "DeviceController",
                          QString("锁定查询，第%1的端口数据有问题").arg(i+1));
        }
        QString macStr = QString::fromLatin1(portInformation.mid(1, 6).toHex(':').toUpper());
        info.lockMac = macStr;
        list.append(info);
    }
    emit portLockStatusReady(list);
    emit operationSuccess(m_pendingOperationName);
}

void DeviceController::handleWriteResponse(const QByteArray &payload)
{
    // 写操作通常返回简单的成功/失败响应
    // 这里简单检查payload是否为空或包含错误码

    if (payload.isEmpty() || payload.size() < 1)
    {
        m_logger->log(Logger::Info, "DeviceController",
                      QString("Write operation succeeded: %1").arg(m_pendingOperationName));
        emit operationSuccess(m_pendingOperationName);
    }
    else
    {
        // 检查是否有错误码
        quint8 result = static_cast<quint8>(payload[0]);
        if (result == 0x01)
        {
            m_logger->log(Logger::Info, "DeviceController",
                          QString("Write operation succeeded: %1").arg(m_pendingOperationName));
            emit operationSuccess(m_pendingOperationName);
        }
        else
        {
            m_logger->log(Logger::Error, "DeviceController",
                          QString("Write operation failed: %1 (error code: 0x%2)")
                          .arg(m_pendingOperationName)
                          .arg(result, 2, 16, QChar('0')));
            emit operationFailed(m_pendingOperationName, QString("Error code: 0x%1").arg(result, 2, 16, QChar('0')));
        }
    }
}

/* 0x06 0x01 连接信息 */
void DeviceController::handlePortConnInfoResponse(const QByteArray &payload)
{
    QList<PortConnInfo> list;
    if (m_parser->parsePortConnInfoResponse(payload, list)) {
        emit portConnInfoReady(list);
        emit operationSuccess(m_pendingOperationName);
    } else {
        emit operationFailed(m_pendingOperationName, "解析失败");
    }
    m_pendingOperation = PendingOperation::None;
}

/* 0x06 0x02 状态计数 */
void DeviceController::handlePortStatusResponse(const QByteArray &payload)
{
    QList<PortStatusInfo> list;
    if (m_parser->parsePortStatusResponse(payload, list)) {
        emit portStatusReady(list);
        emit operationSuccess(m_pendingOperationName);
    } else {
        emit operationFailed(m_pendingOperationName, "解析失败");
    }
    m_pendingOperation = PendingOperation::None;
}

/* 0x06 0x03 错误计数 */
void DeviceController::handlePortErrorResponse(const QByteArray &payload)
{
    QList<PortErrorInfo> list;
    if (m_parser->parsePortErrorResponse(payload, list))
    {
        emit portErrorReady(list);
        emit operationSuccess(m_pendingOperationName);
    }
    else
    {
        emit operationFailed(m_pendingOperationName, "解析失败");
    }
    m_pendingOperation = PendingOperation::None;
}

void DeviceController::handleDeviceLockResponse(const QByteArray &payload)
{
    QString judge = QString::fromLatin1(payload.mid(0,1).toHex());
    QString Word = QString::fromLatin1(payload.mid(1)).trimmed();
    if(judge == "01")
    {
        emit setWordToUnlockLab(Word);
        emit operationSuccess(m_pendingOperationName);
    }
    else
    {
        emit operateError("设备锁定失败");
    }
}

void DeviceController::handleDeviceUnLockResponse(const QByteArray &payload)
{
    QString judge = QString::fromLatin1(payload.mid(0,1).toHex());
    if(judge == "01")
    {
        emit operateOK("设备解锁成功");
        emit operationSuccess(m_pendingOperationName);
    }
    else
    {
        emit operateError("设备解锁失败");
    }
}

void DeviceController::handleDeviceNameReviseResponse(const QByteArray &payload)
{
    QString judge = QString::fromLatin1(payload.mid(0,1).toHex());
    if(judge == "01")
    {
        emit operateOK("设备名修改成功");
    }
    else
    {
        emit operateError("设备名修改成功失败");
    }
}

void DeviceController::handleDeviceConfig(const QByteArray &payload)
{
    QString judge = QString::fromLatin1(payload.mid(0,1).toHex());
    if(judge == "01")
    {
        emit operateOK("配置成功");
        m_pendingOperationName = "Equipment configuration update";
        emit operationSuccess(m_pendingOperationName);
    }
    else
    {
        emit operateError("配置失败");
    }
}

void DeviceController::handlePortLock(const QByteArray &payload)
{
    QString judge = QString::fromLatin1(payload.mid(0,1).toHex());
    if(judge == "01")
    {
        emit operateOK("端口锁定成功");
        emit operationSuccess(m_pendingOperationName);
    }
    else
    {
        emit operateError("端口锁定失败");
    }
}

void DeviceController::handlePortUnLock(const QByteArray &payload)
{
    QString judge = QString::fromLatin1(payload.mid(0,1).toHex());
    if(judge == "01")
    {
        emit operateOK("端口解锁成功");
        emit operationSuccess(m_pendingOperationName);
    }
    else
    {
        emit operateError("端口解锁失败");
    }
}
void DeviceController::handleWriteSerialConfig(const QByteArray &payload)
{
    QString judge = QString::fromLatin1(payload.mid(0,1).toHex());
    if(judge == "01")
    {
        emit operateOK("配置成功");
        emit operationSuccess(m_pendingOperationName);
    }
    else
    {
        emit operateError("配置失败");
    }
}

void DeviceController::handleReadPortMode(const QByteArray &payload)
{
    SerialPortMode mode;
    mode.index = static_cast<int>(payload[0]);
    mode.workMode = static_cast<quint8>(payload[1]);
    m_currentDevice->setSerialPortMode(mode);
    emit operationSuccess(m_pendingOperationName);
}

void DeviceController::handleSelfCheck(const QByteArray &payload)
{
    QString judge = QString::fromLatin1(payload.mid(0,1).toHex());  // 若judge为00，则自检失败。否则就是成功的个数
    if (judge == "00")
    {
        emit operateError("自检失败");
    }
    else
    {
        bool ok;
        QByteArray data = payload.mid(1);
        qDebug() << data.toHex(' ');
        int portNum = QString(judge).toInt(&ok, 16);
        QStringList portNames;
        for (int i = 0; i < data.size(); i++)
        {
            quint8 value = static_cast<quint8>(data.at(i));  // 获取字节的数值
//            QString name = QString::number(value);  // 转换为十进制字符串
            QString name = QString("端口%1").arg(value);
            portNames.append(name);
        }
        QString message = QString("成功自检%1个端口: %2").arg(portNum).arg(portNames.join(", "));
        qDebug() << "portNames：" << portNames;
        emit operateOK(message);
    }
}

void DeviceController::DeviceControllerStartPackageUpgrade(const QByteArray &pkg)
{
    // 建立新的TCP连接，端口号为19001
    m_tcpClient->firmwareUpdate();

    connection = connect(m_tcpClient, &TcpClient::updateConnected, this, [=]{
        UpdateSendData(pkg);
    });
}

void DeviceController::sendUpgradeHandshake()
{
    m_pendingOperation = PendingOperation::FirmwareUpgrade;
    m_pendingOperationName = "Upgrade Handshake";
    m_connManager->sendCommandUpdate(m_fwData);
}

void DeviceController::sendFinalCheck()
{
    m_pendingOperation = PendingOperation::FinalCheck;
    m_pendingOperationName = "Final Check";

    /* 最终校验帧（0x08 0x05），无参数 */
    QByteArray cmd = ProtocolBuilder::buildFinalCheck();
    m_connManager->sendCommand(cmd);
}

bool DeviceController::deviceDisconnected()
{
    m_logger->log(Logger::Debug, "DeviceController", "断开设备");
    bool a = m_tcpClient->disconnectFromServer();
    return a;
}

int DeviceController::getCheckedRows(QTableWidget* tableWidget, int checkboxColumn)
{
    int checkedRows = -1;

    for (int row = 0; row < tableWidget->rowCount(); ++row)
    {
        QTableWidgetItem* item = tableWidget->item(row, checkboxColumn);

        // 检查item存在且是复选框
        if (item && (item->flags() & Qt::ItemIsUserCheckable))
        {
            if (item->checkState() == Qt::Checked)
            {
                checkedRows = row;
            }
        }
    }
    return checkedRows;
}

void DeviceController::disconnectUpdate()
{
    disconnect(connection);
}

void DeviceController::UpdateSendData(const QByteArray &pkg)
{
    /* 0. 状态/计数器初始化 */
    m_pendingOperation  = PendingOperation::PackageUpgrade;
    m_pendingOperationName = "Package Upgrade";
    m_totalPkts = (pkg.size() + 1023) / 1024;   // 总包数
    m_ackPktIdx = 0;    // 接收到的ACK包数
    m_sentPkts = 0;    // 发送的包数

    m_logger->log(Logger::Info, "DeviceController", QString("m_totalPkts %1").arg(m_totalPkts));

    /* 1. 让主窗口弹出遮罩（DeviceController 不负责 new） */
    emit showUpgradeMask(m_totalPkts);   // 信号让 MainWindow 弹出遮罩
    emit upgradeProgress(0);             // 初始 0%


    // 先发送文件大小 4Byte
    quint32 fileSize = static_cast<quint32>(pkg.size());
    m_logger->log(Logger::Info, "DeviceController", QString("fileSize %1").arg(fileSize));
    quint32 bigEndianSize = qToBigEndian(fileSize);
    QByteArray sizeBytes(reinterpret_cast<const char*>(&bigEndianSize), 4);
    m_fwData = sizeBytes;
    sendUpgradeHandshake();
    m_logger->log(Logger::Debug, "DeviceController", QString("startPackageUpgrade m_fwData %1").arg(QString(m_fwData.toHex(' '))));

    /* 2. 发送帧 */
    while (m_sentPkts != m_totalPkts)
    {
        // 要发送的包序号（从0开始）
        int start = m_sentPkts * 1024; // 计算当前包的起始位置
        // 计算当前包的长度：最后1包可能不足1024，取实际长度
        int len = qMin(1024, pkg.size() - start);
        m_fwData = pkg.mid(start, len);
        sendUpgradeHandshake();
        m_sentPkts++;
        // 计算百分比进度
        int progress = static_cast<int>((static_cast<double>(m_sentPkts) / m_totalPkts) * 100);
        emit upgradeProgress(progress);
        
        // 处理事件，让UI有机会更新
        QCoreApplication::processEvents();
        
        // 添加小延迟，确保进度条能够平滑更新
        QThread::msleep(5);
    }
    // 确保最终进度为100%
    emit upgradeProgress(100);
    QCoreApplication::processEvents();
    // 升级成功
    emit updateOK();
}
