#include "ConfigurationData.h"
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDebug>
#include "../../utils/Logger.h"

ConfigurationData::ConfigurationData()
    : m_ttyMajor(34)
    , m_calloutMajor(39)
{
}

QPair<bool, QString> ConfigurationData::parseFromFile(const QString& path)
{
    m_configPath = path;
    m_devices.clear();
    
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString errorMsg = QString("Cannot open file: %1").arg(file.errorString());
        Logger::instance()->log(Logger::Error, "ConfigurationData", QString("Failed to read configuration file %1: %2").arg(path).arg(errorMsg));
        return QPair<bool, QString>(false, errorMsg);
    }
    
    QTextStream in(&file);
    int lineNum = 0;
    
    while (!in.atEnd()) {
        QString line = in.readLine();
        lineNum++;
        
        QString trimmed = line.trimmed();
        
        // 跳过空行和注释
        if (trimmed.isEmpty() || trimmed.startsWith('#')) {
            continue;
        }
        
        // 解析主设备号
        if (trimmed.startsWith("ttymajor=")) {
            bool ok = false;
            int major = trimmed.mid(9).toInt(&ok);
            if (ok) {
                m_ttyMajor = major;
                Logger::instance()->log(Logger::Debug, "ConfigurationData", QString("Parsed tty_major: %1").arg(m_ttyMajor));
            } else {
                Logger::instance()->log(Logger::Warning, "ConfigurationData", QString("Invalid ttymajor line %1: %2").arg(lineNum).arg(line));
            }
            continue;
        }
        
        if (trimmed.startsWith("calloutmajor=")) {
            bool ok = false;
            int major = trimmed.mid(13).toInt(&ok);
            if (ok) {
                m_calloutMajor = major;
                Logger::instance()->log(Logger::Debug, "ConfigurationData", QString("Parsed callout_major: %1").arg(m_calloutMajor));
            } else {
                Logger::instance()->log(Logger::Warning, "ConfigurationData", QString("Invalid calloutmajor line %1: %2").arg(lineNum).arg(line));
            }
            continue;
        }
        
        // 解析端口配置行
        bool ok = false;
        PortModel port = PortModel::fromConfigLine(line, &ok);
        if (ok) {
            // 添加到设备（如果需要则创建设备）
            QString ip = port.ip();
            auto deviceIt = m_devices.find(ip);
            if (deviceIt == m_devices.end()) {
                deviceIt = m_devices.insert(ip, ConfigDeviceModel(ip));
            }
            deviceIt->addPort(port);
            Logger::instance()->log(Logger::Debug, "ConfigurationData", QString("Added port: %1 for device %2").arg(port.ttyName()).arg(ip));
        }
    }
    
    file.close();
    
    // 推断每个设备的端口容量
    for (auto it = m_devices.begin(); it != m_devices.end(); ++it) {
        int capacity = it.value().inferPortCapacity();
        it.value().setPortCapacity(capacity);
        Logger::instance()->log(Logger::Debug, "ConfigurationData", QString("Device %1: inferred capacity = %2 ports").arg(it.key()).arg(capacity));
    }
    
    Logger::instance()->log(Logger::Debug, "ConfigurationData", QString("Parsed configuration from %1: %2 devices, %3 ports").arg(path).arg(m_devices.size()).arg(totalPortCount()));
    
    return QPair<bool, QString>(true, "");
}

QPair<bool, QString> ConfigurationData::writeToFile(const QString& path)
{
    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString tempPath = tempDir + "/config_temp";

    QFile file(tempPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QString errorMsg = QString("Cannot write file: %1").arg(file.errorString());
        Logger::instance()->log(Logger::Error, "ConfigurationData", QString("Failed to write configuration file %1: %2").arg(path).arg(errorMsg));
        return QPair<bool, QString>(false, errorMsg);
    }

    QTextStream out(&file);
    
    // 写入文件头
    out << "#=========================================================#\n";
    out << "#   This configuration file is created by WQ NPort      #\n";
    out << "#   Administrator Program automatically, please do not    #\n";
    out << "#   modify this file by yourself.                         #\n";
    out << "#=========================================================#\n";
    
    // 写入主设备号
    out << "ttymajor=" << m_ttyMajor << "\n";
    out << "calloutmajor=" << m_calloutMajor << "\n";
    
    // 写入列标题
    out << "#[Minor] [ServerIP]\t[data]\t[cmd]\t[FIFO]\t[SSL]\t"
        << "[ttyName] [coutName] [interface][mode][BackIP]\t\n";
    
    Logger::instance()->log(Logger::Debug, "ConfigurationData", "out内容写入完毕");

    // 按IP和minor号排序写入端口配置
    QStringList sortedIps = m_devices.keys();
    sortedIps.sort();
    
    for (const QString& ip : sortedIps)
    {
        auto deviceIt = m_devices.constFind(ip);
        const ConfigDeviceModel& device = deviceIt.value();
        
        // 按minor号排序端口
        QList<PortModel> sortedPorts = device.ports();
        std::sort(sortedPorts.begin(), sortedPorts.end(),
                  [](const PortModel& a, const PortModel& b)
        {
            return a.minor() < b.minor();
        });
        
        for (const PortModel& port : sortedPorts) {
            out << port.toConfigLine() << "\n";
        }
    }
    
    Logger::instance()->log(Logger::Debug, "ConfigurationData", "ports内容写入完毕");

    file.close();
    
    QProcess process;
    QStringList arguments;
    arguments << "cp" << tempPath << path;

    process.start("pkexec", arguments);
    if (!process.waitForStarted()) {
        return QPair<bool, QString>(false, "Failed to start privileged copy operation");
    }

    if (!process.waitForFinished(10000)) { // 10秒超时
        process.kill();
        return QPair<bool, QString>(false, "Privileged copy operation timed out");
    }

    if (process.exitCode() != 0) {
        QString errorOutput = process.readAllStandardError();
        return QPair<bool, QString>(false,
                                    QString("Failed to copy config file: %1").arg(errorOutput));
    }

    // 3. 清理临时文件
    QFile::remove(tempPath);

    Logger::instance()->log(Logger::Debug, "ConfigurationData", "文件复制完毕");

    return QPair<bool, QString>(true, "");
}


void ConfigurationData::addDevice(const ConfigDeviceModel& device)
{
    m_devices.insert(device.ip(), device);
}

bool ConfigurationData::removeDevice(const QString& ip)
{
    if (m_devices.contains(ip)) {
        m_devices.remove(ip);
        Logger::instance()->log(Logger::Debug, "ConfigurationData", QString("Removed device %1").arg(ip));
        return true;
    }
    Logger::instance()->log(Logger::Warning, "ConfigurationData", QString("Device %1 not found").arg(ip));
    return false;
}

ConfigDeviceModel* ConfigurationData::getDevice(const QString& ip)
{
    auto it = m_devices.find(ip);
    if (it != m_devices.end()) {
        return &(it.value());
    }
    return nullptr;
}

const ConfigDeviceModel* ConfigurationData::getDevice(const QString& ip) const
{
    QMap<QString, ConfigDeviceModel>::const_iterator it = m_devices.constFind(ip);
    if (it != m_devices.constEnd())
    {
        return &(it.value());
    }
    return nullptr;
}

QList<ConfigDeviceModel> ConfigurationData::getDevices() const
{
    return m_devices.values();
}

QSet<int> ConfigurationData::usedMinors() const
{
    QSet<int> minors;
    for (const ConfigDeviceModel& device : m_devices)
    {
        for (const PortModel& port : device.ports())
        {
            minors.insert(port.minor());
        }
    }
    return minors;
}

QSet<QString> ConfigurationData::usedTtyNames() const
{
    QSet<QString> ttyNames;
    for (const ConfigDeviceModel& device : m_devices) {
        for (const PortModel& port : device.ports()) {
            ttyNames.insert(port.ttyName());
        }
    }
    return ttyNames;
}

int ConfigurationData::nextAvailableMinor() const
{
    QSet<int> used = usedMinors();
    for (int minor = 0; minor < 256; ++minor)
    {
        if (!used.contains(minor))
        {
            return minor;
        }
    }
    return -1; // All minors used
}

QString ConfigurationData::nextAvailableTty() const
{
    QSet<QString> used = usedTtyNames();
    for (int index = 0; index < 256; ++index)
    {
        QString ttyName = QString("ttyw%1").arg(index, 2, 16, QChar('0'));
        if (!used.contains(ttyName))
        {
            return ttyName;
        }
    }
    return QString(); // All TTY names used
}

int ConfigurationData::totalPortCount() const
{
    int count = 0;
    for (const ConfigDeviceModel& device : m_devices) {
        count += device.portCount();
    }
    return count;
}
