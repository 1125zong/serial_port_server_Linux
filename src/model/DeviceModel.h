#ifndef DEVICEMODEL_H
#define DEVICEMODEL_H

#include <QObject>
#include <QString>
#include <QList>
#include "SerialPortConfig.h"
#include "NetworkConfig.h"
#include "../protocol/data.h"

/**
 * @brief 设备数据模型
 * 
 * 存储设备的所有信息,包括基本信息、网络配置和串口配置
 * 当数据变化时发出相应的信号通知观察者
 */
class DeviceModel : public QObject {
    Q_OBJECT
    
public:
    // 构造函数
    explicit DeviceModel(QObject *parent = nullptr);
    
    // 设备基本信息访问器
    QString deviceModel() const;
    QString serialNumber() const;
    QString deviceName() const;
    QString firmwareVersion() const;
    
    // 网络信息访问器
    QString lan1Ip() const;
    QString lan1Mac() const;
    QString lan2Ip() const;
    QString lan2Mac() const;
    
    // 设备状态访问器
    QPORT_DeviceState deviceState() const;
    QPORT_DeviceLockState lockState() const;
    
    // 串口配置访问器
    QList<SerialPortConfig> serialPorts() const;
    SerialPortConfig serialPort(int index) const;
    SerialPortMode seriaPortModes() const;
    int serialPortCount() const;
    
    // 完整设备信息访问器
    DeviceInfo deviceInfo() const;
    NetworkConfig networkConfig() const;
    
    // 修改器 - 设备基本信息
    void setDeviceInfo(const DeviceInfo &info);
    void setDeviceName(const QString &name);
    void setDeviceState(QPORT_DeviceState state);
    void setLockState(QPORT_DeviceLockState state);

    // 修改器 - 网络配置
    void setNetworkConfig(const NetworkConfig &config);
    void setSerialPorts(const QList<SerialPortConfig> &serialPorts);
    void setSerialPortMode(const SerialPortMode &serialPortModes);

    quint8 validLanCount() const;
    QPORT_LCDState    lcdState()      const;
    QPORT_PowerState  power1State()   const;
    QPORT_PowerState  power2State()   const;
    int            portCount()                const;
    PortStatus     port(int index)            const;   // index 从 1 开始
    QList<PortStatus> ports()                 const;
    
signals:
    // 数据变化信号
    void deviceInfoChanged();
    void networkConfigChanged();
    void deviceStateChanged(QPORT_DeviceState state);
    void lockStateChanged(QPORT_DeviceLockState state);
    
private:
    DeviceInfo m_deviceInfo;
    QList<SerialPortConfig> m_serialPorts;
    NetworkConfig m_networkConfig;
    SerialPortMode m_serialPortMode;
};

#endif // DEVICEMODEL_H
