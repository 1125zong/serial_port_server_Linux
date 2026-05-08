#ifndef _CONFIGDEVICEMODEL_H
#define _CONFIGDEVICEMODEL_H

#include "PortModel.h"
#include <QString>
#include <QList>

/**
 * @brief Represents a single NPort device with all its ports
 * 
 * Groups ports by IP address and manages device-level properties.
 */
class ConfigDeviceModel
{
public:
    explicit ConfigDeviceModel(const QString& ip, int portCapacity = 16);
    
    // ConfigDevice info
    QString ip() const { return m_ip; }
    int portCapacity() const { return m_portCapacity; }
    void setPortCapacity(int capacity);
    int inferPortCapacity() const;
    
    // Port management
    void addPort(const PortModel& port);
    bool removePort(int minor);
    int portCount() const { return m_ports.size(); }
    QList<PortModel> ports() const { return m_ports; }
    QList<PortModel>& portsRef() { return m_ports; }
    
    PortModel* portByMinor(int minor);
    const PortModel* portByMinor(int minor) const;
    PortModel* portByTtyName(const QString& ttyName);
    const PortModel* portByTtyName(const QString& ttyName) const;
    
private:
    QString m_ip;
    int m_portCapacity;
    QList<PortModel> m_ports;
};

#endif // _CONFIGDEVICEMODEL_H
