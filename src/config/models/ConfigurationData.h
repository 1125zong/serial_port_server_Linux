#ifndef CONFIGURATIONDATA_H
#define CONFIGURATIONDATA_H

#include "ConfigDeviceModel.h"
#include <QString>
#include <QMap>
#include <QSet>
#include <QList>
#include <QPair>
#include <QTemporaryFile>     // 添加：临时文件支持
#include <QStandardPaths>      // 添加：可执行文件路径查找
#include <QProcess>            // 添加：系统进程执行

/**
 * @brief Represents the complete configuration state from wq_nportd.cf
 * 
 * Manages all devices and ports, handles file I/O, and provides
 * resource allocation for minor numbers and TTY names.
 */
class ConfigurationData
{
public:
    ConfigurationData();
    
    // File I/O - returns <success, errorMessage>
    QPair<bool, QString> parseFromFile(const QString& path);
    QPair<bool, QString> writeToFile(const QString& path);
    
    // Device management
    void addDevice(const ConfigDeviceModel& device);
    bool removeDevice(const QString& ip);
    ConfigDeviceModel* getDevice(const QString& ip);
    const ConfigDeviceModel* getDevice(const QString& ip) const;
    QList<ConfigDeviceModel> getDevices() const;
    
    // Resource allocation
    QSet<int> usedMinors() const;
    QSet<QString> usedTtyNames() const;
    int nextAvailableMinor() const;
    QString nextAvailableTty() const;
    
    // Statistics
    int totalPortCount() const;
    int deviceCount() const { return m_devices.size(); }
    
    // Major numbers
    int ttyMajor() const { return m_ttyMajor; }
    int calloutMajor() const { return m_calloutMajor; }
    void setTtyMajor(int major) { m_ttyMajor = major; }
    void setCalloutMajor(int major) { m_calloutMajor = major; }
    
    // Configuration path
    QString configPath() const { return m_configPath; }
    
private:
    int m_ttyMajor;
    int m_calloutMajor;
    QMap<QString, ConfigDeviceModel> m_devices;
    QString m_configPath;
};

#endif // CONFIGURATIONDATA_H
