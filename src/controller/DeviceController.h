#ifndef DEVICECONTROLLER_H
#define DEVICECONTROLLER_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QInputDialog>
#include <QList>
#include <QDebug>
#include <QProgressBar>
#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QApplication>
#include <QCryptographicHash>
#include <QMessageBox>
#include <QTableWidget>
#include "../model/DeviceModel.h"
#include "../model/NetworkConfig.h"
#include "../model/SerialPortConfig.h"
#include "../communication/TcpClient.h"
#include "../communication/UdpDiscovery.h"
#include "../communication/ConnectionManager.h"
#include "../protocol/ProtocolParser.h"
#include "../protocol/ProtocolBuilder.h"
#include "../utils/Logger.h"
#include "../protocol/data.h"

/**
 * @brief 设备控制器
 *
 * 协调Model、View和Communication层
 * 实现Requirements 1.1-1.5中的业务逻辑控制
 */
class DeviceController : public QObject {
    Q_OBJECT


public:
    explicit DeviceController(QObject *parent = nullptr);
    ~DeviceController();

    DeviceModel *m_currentDevice;

    void disconnectUpdate();
    // 设备发现
    void searchDevices();

    // 断开设备
    bool deviceDisconnected();

    // 连接设备
    void connectToDevice(const QString &ip, quint16 port = 4000);
    void disconnectFromDevice();

    // 读取设备信息
    void readOverview();
    void readNetworkConfig();
    void readSerialConfig(int portIndex = 0xFFFF);
    void readPortLockStatus();

    // 写入配置
    void writeDeviceName(const QString &name);
    void writeNetworkConfig(const NetworkConfig &config);
    void writeSerialConfig(const SerialPortConfig &config,int i);
    void writeMultiSerialConfig(const QList<SerialPortConfig> &configs);

    // 设备管理
    void lockPort(int portIndex);
    void unlockPort(int portIndex);
    void lockDevice();
    void unlockDevice(const QString &code);
    void factoryReset();
    void reboot();

    // 获取当前设备模型
    DeviceModel* currentDevice() const;
    void selfCheckPort(quint8 portByte);
    void writeModeConfig(int port, quint8 mode, const QVariantMap &params,const SerialPortConfig &config,int i);
    void setPortMirror(const QByteArray &pkt);
    void setWatchDog(const QByteArray &pkt);
    void startFirmwareUpgrade(const QString &file);

    void readPortConnInfo();
    void readPortStatus();
    void readPortError();
    void lockPortsWithSync(const QByteArray &bitmap);   //  bitmap.size() == 32
    void unlockPortsWithSync(const QByteArray &bitmap);
    void DeviceControllerStartPackageUpgrade(const QByteArray &pkg);
    static void logSend(const QByteArray &data);

    void appendSyncBitmap(const QByteArray &bitmap);
    void buildPortReset(const QByteArray &bitmap);
    void queryPortMode(const int portIndex);
    void queryPortSpecifiedMode(const QByteArray &mode);
    int getCheckedRows(QTableWidget* tableWidget, int checkboxColumn);

    signals:
        // 设备发现
    void deviceDiscovered(const DeviceInfo &device);
    void searchCompleted(int count);
    void udpBindSucceeded(quint16 port);

    // 连接状态
    void connected();
    void disconnected();
    void connectionError(const QString &error);

    // 操作结果
    void operationSuccess(const QString &operation);
    void operationFailed(const QString &operation, const QString &error);

    // 数据更新
    void serialConfigUpdated();
    /* 固件升级握手请求（UI 负责实际发送） */
    void upgradeHandshakeRequested(const QString &filePath);
    void portConnInfoReady(const QList<PortConnInfo> &);
    void portStatusReady(const QList<PortStatusInfo> &);
    void portErrorReady(const QList<PortErrorInfo> &);
    void portLockStatusReady(const QList<PortLockInfo> &list);
    void upgradeProgress(int percent);   // 0~100
    void updateOK();

    void upgradeSuccess();
    void upgradeFailed(const QString &error);
    void showUpgradeMask(int totalPkts);   // 让主窗口弹出遮罩
    void setWordToUnlockLab(const QString &word);

    // 设备配置修改
    void deviceConfigOK();
    void deviceConfigError();

    void operateError(const QString &action);
    void operateOK(const QString &action);

private slots:
    void onFrameReceived(const QByteArray &frame);
    void onDeviceDiscovered(const DeviceInfo &device);
    void onCommandTimeout();
    void onTcpConnected();
    void onTcpDisconnected();
    void onTcpError(const QString &error);
    void onSearchCompleted(int count);
    void onSearchError(const QString &error);
    void UpdateSendData(const QByteArray &pkg);


private:
    void handleOverviewResponse(const QByteArray &payload);
    void handleNetworkConfigResponse(const QByteArray &payload);
    void handleSerialConfigResponse(const QByteArray &payload);
    void handlePortLockStatusResponse(const QByteArray &payload);
    void handleWriteResponse(const QByteArray &payload);
    void handlePortConnInfoResponse(const QByteArray &payload);
    void handlePortStatusResponse(const QByteArray &payload);
    void handlePortErrorResponse(const QByteArray &payload);
    void handleDeviceLockResponse(const QByteArray &payload);
    void handleDeviceUnLockResponse(const QByteArray &payload);
    void handleDeviceNameReviseResponse(const QByteArray &payload);
    void handleDeviceConfig(const QByteArray &payload);
    void handlePortLock(const QByteArray &payload);
    void handlePortUnLock(const QByteArray &payload);
    void handleWriteSerialConfig(const QByteArray &payload);
    void handleReadPortMode(const QByteArray &payload);
    void handleSelfCheck(const QByteArray &payload);
    void handleWriteWatchDog(const QByteArray &payload);

    void sendUpgradeHandshake();   // 发送升级握手帧
    void sendFinalCheck();

    TcpClient *m_tcpClient;
    UdpDiscovery *m_udpDiscovery;
    ConnectionManager *m_connManager;
    ProtocolParser *m_parser;
    Logger *m_logger;
    uint32_t m_ackPktIdx = 0;   // 已收到 ACK 包数
    uint32_t m_totalPkts = 0;   // 总包数
    QByteArray m_fwData;

    quint32 m_sentPkts;    // 已发送包数（可选）
    // 跟踪当前操作
    enum class PendingOperation {
        None,
        ReadOverview,
        ReadNetworkConfig,
        ReadSerialConfig,
        ReadPortLockStatus,
        WriteDeviceName,
        WriteNetworkConfig,
        WriteSerialConfig,
        WriteMultiSerialConfig,
        WriteMode,
        FinalCheck,
        ResetPort,
        LockPort,
        UnlockPort,
        LockDevice,
        UnlockDevice,
        FactoryReset,
        Reboot,
        SelfCheck,
        WriteMirror,
        WriteWatchDog,
        FirmwareUpgrade,
        PackageUpgrade,
        ReadPortConnInfo,
        ReadPortStatus,
        ReadPortError,
        ReadPortMode,
        ReadPortSpecifiedMode
    };

    PendingOperation m_pendingOperation;
    QString m_pendingOperationName;
    QMetaObject::Connection connection;
};

#endif // DEVICECONTROLLER_H
