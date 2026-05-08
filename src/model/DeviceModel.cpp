#include "DeviceModel.h"
#include "../utils/Logger.h"

DeviceModel::DeviceModel(QObject *parent)
    : QObject(parent)
{
    // 初始化默认值
    m_deviceInfo.deviceState = QPORT_DeviceState::WorkingMode;
    m_deviceInfo.devLockState = QPORT_DeviceLockState::Unlocked;
    m_deviceInfo.validLanCount = 0;
    
    Logger::instance()->log(Logger::Info, "DeviceModel", "设备模型初始化完成");
}

// 设备基本信息访问器
QString DeviceModel::deviceModel() const {
    return m_deviceInfo.deviceModel;
}

QString DeviceModel::serialNumber() const {
    return m_deviceInfo.serialNum;
}

QString DeviceModel::deviceName() const
{
    return m_deviceInfo.deviceName;
}

QString DeviceModel::firmwareVersion() const {
    return m_deviceInfo.firmwareVer;
}

// 网络信息访问器
QString DeviceModel::lan1Ip() const {
    return m_deviceInfo.lan1Ip;
}

QString DeviceModel::lan1Mac() const {
    return m_deviceInfo.lan1Mac;
}

QString DeviceModel::lan2Ip() const {
    return m_deviceInfo.lan2Ip;
}

QString DeviceModel::lan2Mac() const {
    return m_deviceInfo.lan2Mac;
}

// 设备状态访问器
QPORT_DeviceState DeviceModel::deviceState() const {
    return m_deviceInfo.deviceState;
}

QPORT_DeviceLockState DeviceModel::lockState() const {
    return m_deviceInfo.devLockState;
}

// 串口配置访问器
QList<SerialPortConfig> DeviceModel::serialPorts() const {
    return m_serialPorts;
}

SerialPortConfig DeviceModel::serialPort(int index) const
{
    // 查找指定索引的串口配置
    for (const SerialPortConfig &config : m_serialPorts)
    {
        if (config.index == index)
        {
            return config;
        }
    }
    // 如果未找到,返回默认配置
    SerialPortConfig defaultConfig;
    defaultConfig.index = index;
    return defaultConfig;
}

SerialPortMode DeviceModel::seriaPortModes() const
{
    return m_serialPortMode;
}


int DeviceModel::serialPortCount() const {
    return m_serialPorts.size();
}

// 完整设备信息访问器
DeviceInfo DeviceModel::deviceInfo() const {
    return m_deviceInfo;
}

NetworkConfig DeviceModel::networkConfig() const
{
    return m_networkConfig;
}

// 修改器 - 设备基本信息
void DeviceModel::setDeviceInfo(const DeviceInfo &info)
{
    m_deviceInfo = info;
    emit deviceInfoChanged();
    
    Logger::instance()->log(Logger::Info, "DeviceModel", QString("设备信息更新: 型号=%1, 序列号=%2, 名称=%3")
                           .arg(info.deviceModel)
                           .arg(info.serialNum)
                           .arg(info.deviceName));
}

void DeviceModel::setDeviceName(const QString &name)
{
    if (m_deviceInfo.deviceName != name) {
        QString oldName = m_deviceInfo.deviceName;
        m_deviceInfo.deviceName = name;
        emit deviceInfoChanged();
        
        Logger::instance()->log(Logger::Info, "DeviceModel", QString("设备名称变更: 从 %1 到 %2")
                               .arg(oldName)
                               .arg(name));
    }
}

void DeviceModel::setDeviceState(QPORT_DeviceState state) {
    if (m_deviceInfo.deviceState != state) {
        QPORT_DeviceState oldState = m_deviceInfo.deviceState;
        m_deviceInfo.deviceState = state;
        emit deviceStateChanged(state);
        
        Logger::instance()->log(Logger::Info, "DeviceModel", QString("设备状态变更: 从 %1 到 %2")
                               .arg(static_cast<int>(oldState))
                               .arg(static_cast<int>(state)));
    }
}

void DeviceModel::setLockState(QPORT_DeviceLockState state)
{
    if (m_deviceInfo.devLockState != state)
    {
        QPORT_DeviceLockState oldState = m_deviceInfo.devLockState;
        m_deviceInfo.devLockState = state;
        emit lockStateChanged(state);
        
        Logger::instance()->log(Logger::Info, "DeviceModel", QString("设备锁定状态变更: 从 %1 到 %2")
                               .arg(static_cast<int>(oldState))
                               .arg(static_cast<int>(state)));
    }
}

// 修改器 - 网络配置
void DeviceModel::setNetworkConfig(const NetworkConfig &config)
{
    m_networkConfig = config;
}


quint8 DeviceModel::validLanCount() const
{
    return m_deviceInfo.validLanCount;
}

QPORT_LCDState DeviceModel::lcdState() const
{
    return m_deviceInfo.lcdState;
}

QPORT_PowerState DeviceModel::power1State() const
{
    return m_deviceInfo.power1State;
}

QPORT_PowerState DeviceModel::power2State() const
{
    return m_deviceInfo.power2State;
}

int DeviceModel::portCount() const
{
    return m_deviceInfo.ports.size();
}

PortStatus DeviceModel::port(int index) const
{
    for (const PortStatus &p : m_deviceInfo.ports)
        if (p.portNum == index)      // 按设备端口号匹配
            return p;
    return PortStatus();             // 没找到返回默认
}

QList<PortStatus> DeviceModel::ports() const
{
    return m_deviceInfo.ports;
}

void DeviceModel::setSerialPorts(const QList<SerialPortConfig> &serialPorts)
{
    m_serialPorts = serialPorts;
    
    Logger::instance()->log(Logger::Info, "DeviceModel", QString("串口配置更新 (%1 个端口)").arg(serialPorts.size()));
}

void DeviceModel::setSerialPortMode(const SerialPortMode &serialPortModes)
{
    m_serialPortMode = serialPortModes;
}



