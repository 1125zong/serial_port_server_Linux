#include "ConfigDeviceModel.h"
#include <QDebug>
#include <algorithm>
#include "../../utils/Logger.h"

ConfigDeviceModel::ConfigDeviceModel(const QString& ip, int portCapacity)
    : m_ip(ip)
    , m_portCapacity(portCapacity)
{
}

void ConfigDeviceModel::setPortCapacity(int capacity)
{
    // 验证容量
    QList<int> validCapacities = {1, 4, 8, 16};
    if (!validCapacities.contains(capacity)) {
        Logger::instance()->log(Logger::Warning, "ConfigDeviceModel", QString("Invalid port capacity %1, defaulting to 16").arg(capacity));
        capacity = 16;
    }
    m_portCapacity = capacity;
    Logger::instance()->log(Logger::Debug, "ConfigDeviceModel", QString("Device %1 port capacity set to %2").arg(m_ip).arg(capacity));
}

int ConfigDeviceModel::inferPortCapacity() const
{
    if (m_ports.isEmpty()) {
        return 16; // Default to maximum
    }
    
    // 找到最高的端口号
    int maxPortNum = 0;
    for (const PortModel& port : m_ports) {
        int portNum = 0;
        
        // 从data_port计算端口号
        if (port.dataPort() >= 950 && port.dataPort() <= 965) {
            portNum = port.dataPort() - 950 + 1; // Port 1-16
        } else if (port.dataPort() >= 966 && port.dataPort() <= 981) {

        } else {
            // 回退：使用minor + 1
            portNum = port.minor() + 1;
        }
        
        maxPortNum = std::max(maxPortNum, portNum);
    }
    
    // 根据最高端口确定容量
    if (maxPortNum <= 1) return 1;
    if (maxPortNum <= 4) return 4;
    if (maxPortNum <= 8) return 8;
    if (maxPortNum <= 16) return 16;
    return 16;
}

void ConfigDeviceModel::addPort(const PortModel& port)
{
    if (port.ip() != m_ip) {
        Logger::instance()->log(Logger::Warning, "ConfigDeviceModel", QString("Port IP %1 does not match device IP %2").arg(port.ip()).arg(m_ip));
    }
    m_ports.append(port);
    Logger::instance()->log(Logger::Debug, "ConfigDeviceModel", QString("Added port with minor %1 to device %2").arg(port.minor()).arg(m_ip));
}

bool ConfigDeviceModel::removePort(int minor)
{
    for (int i = 0; i < m_ports.size(); ++i) {
        if (m_ports[i].minor() == minor) {
            m_ports.removeAt(i);
            Logger::instance()->log(Logger::Debug, "ConfigDeviceModel", QString("Removed port with minor %1 from device %2").arg(minor).arg(m_ip));
            return true;
        }
    }
    
    Logger::instance()->log(Logger::Warning, "ConfigDeviceModel", QString("Port with minor %1 not found in device %2").arg(minor).arg(m_ip));
    return false;
}

PortModel* ConfigDeviceModel::portByMinor(int minor)
{
    for (int i = 0; i < m_ports.size(); ++i) {
        if (m_ports[i].minor() == minor) {
            return &m_ports[i];
        }
    }
    return nullptr;
}

const PortModel* ConfigDeviceModel::portByMinor(int minor) const
{
    for (const PortModel& port : m_ports) {
        if (port.minor() == minor) {
            return &port;
        }
    }
    return nullptr;
}

PortModel* ConfigDeviceModel::portByTtyName(const QString& ttyName)
{
    for (int i = 0; i < m_ports.size(); ++i) {
        if (m_ports[i].ttyName() == ttyName) {
            return &m_ports[i];
        }
    }
    return nullptr;
}

const PortModel* ConfigDeviceModel::portByTtyName(const QString& ttyName) const
{
    for (const PortModel& port : m_ports) {
        if (port.ttyName() == ttyName) {
            return &port;
        }
    }
    return nullptr;
}
