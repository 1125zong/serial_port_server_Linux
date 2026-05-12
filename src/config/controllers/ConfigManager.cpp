#include "ConfigManager.h"
#include "../utils/InputValidator.h"
#include "../utils/PortCalculator.h"
#include "../../utils/Logger.h"
#include <QFileInfo>
#include <QSet>
#include <QStringList>
#include <algorithm>

ConfigManager::ConfigManager()
{
}

ConfigManager::~ConfigManager()
{
}

ConfigManager::OperationResult ConfigManager::loadConfiguration(const QString& configFilePath)
{
    m_configFilePath = configFilePath;

    QPair<bool, QString> result = m_config.parseFromFile(configFilePath);
    if (!result.first) {
        return OperationResult(false, QString("Failed to load configuration: %1").arg(result.second));
    }

    return OperationResult(true, "");
}

ConfigManager::OperationResult ConfigManager::saveConfiguration(const QString& configFilePath)
{
    if (configFilePath.isEmpty())
    {
        return OperationResult(false, "Configuration file path is empty");
    }

    QPair<bool, QString> result = m_config.writeToFile(configFilePath);
    if (!result.first)
    {
        return OperationResult(false, QString("Failed to save configuration: %1").arg(result.second));
    }

    m_configFilePath = configFilePath;
    Logger::instance()->log(Logger::Debug, "ConfigManager",
                            QString("saveConfiguration saved to: %1").arg(configFilePath));
    return OperationResult(true, "");
}

ConfigManager::OperationResult ConfigManager::validateConfiguration()
{
    QSet<int> minors = m_config.usedMinors();
    int totalPorts = m_config.totalPortCount();
    if (minors.size() != totalPorts) {
        return OperationResult(false, "Configuration has duplicate minor numbers");
    }

    QSet<QString> ttyNames = m_config.usedTtyNames();
    if (ttyNames.size() != totalPorts) {
        return OperationResult(false, "Configuration has duplicate TTY names");
    }

    QList<ConfigDeviceModel> devices = m_config.getDevices();
    for (const ConfigDeviceModel& device : devices) {
        auto ipResult = InputValidator::validateIpAddress(device.ip());
        if (!ipResult.first) {
            return OperationResult(false, QString("Invalid device IP %1: %2")
                                   .arg(device.ip()).arg(ipResult.second));
        }

        QList<PortModel> ports = device.ports();
        for (const PortModel& port : ports) {
            auto minorResult = InputValidator::validateMinorNumber(port.minor());
            if (!minorResult.first) {
                return OperationResult(false, QString("Invalid minor number %1: %2")
                                       .arg(port.minor()).arg(minorResult.second));
            }

            auto dataPortResult = InputValidator::validatePortNumber(port.dataPort());
            if (!dataPortResult.first) {
                return OperationResult(false, QString("Invalid data port %1: %2")
                                       .arg(port.dataPort()).arg(dataPortResult.second));
            }

            auto cmdPortResult = InputValidator::validatePortNumber(port.cmdPort());
            if (!cmdPortResult.first) {
                return OperationResult(false, QString("Invalid command port %1: %2")
                                       .arg(port.cmdPort()).arg(cmdPortResult.second));
            }

            auto ttyResult = InputValidator::validateTtyName(port.ttyName());
            Logger::instance()->log(Logger::Debug, "ConfigManager",
                                    QString("port.ttyName(): %1").arg(port.ttyName()));
            if (!ttyResult.first) {
                return OperationResult(false, QString("Invalid TTY name %1: %2")
                                       .arg(port.ttyName()).arg(ttyResult.second));
            }
        }
    }

    return OperationResult(true, "Configuration is valid");
}

ConfigManager::OperationResult ConfigManager::batchAddPorts(const QString& deviceIp, int portCount,
                                                            int startPort, bool ssl,
                                                            bool redundantMode, const QString& backupIp)
{
    Q_UNUSED(ssl);
    Logger::instance()->log(Logger::Debug, "ConfigManager", "batchAddPorts");

    auto ipResult = InputValidator::validateIpAddress(deviceIp);
    if (!ipResult.first) {
        return OperationResult(false, ipResult.second);
    }

    auto countResult = InputValidator::validatePortCount(portCount);
    if (!countResult.first) {
        return OperationResult(false, countResult.second);
    }

    auto portResult = InputValidator::validatePortNumber(startPort);
    if (!portResult.first) {
        return OperationResult(false, portResult.second);
    }

    if (redundantMode && backupIp.isEmpty()) {
        return OperationResult(false, "Backup IP is required for redundant mode");
    }

    if (redundantMode) {
        auto backupIpResult = InputValidator::validateIpAddress(backupIp);
        if (!backupIpResult.first) {
            return OperationResult(false, QString("Invalid backup IP: %1").arg(backupIpResult.second));
        }
    }

    OperationResult backupResult = createBackupBeforeModification();
    if (!backupResult.first)
    {
        return backupResult;
    }

    QPair<int, int> ports = PortCalculator::calculatePorts(1, startPort);
    PortModel port;
    port.setIp(deviceIp);
    port.setDataPort(ports.first);
    port.setCmdPort(ports.second);
    port.setInterface("0");
    port.setMode(redundantMode ? 1 : 0);
    if (redundantMode) {
        port.setBackupIp(backupIp);
    }

    OperationResult execResult = executeMxaddsvrForPort(port, portCount);
    if (!execResult.first)
    {
        m_config.parseFromFile(m_configFilePath);
        return OperationResult(false, QString("Failed to execute mxaddsvr: %1\nBackup: %2")
                               .arg(execResult.second)
                               .arg(m_lastBackupPath));
    }

    m_config.parseFromFile(m_configFilePath);
    OperationResult nodeResult = verifyDeviceNodes(deviceIp);
    if (!nodeResult.first) {
        return nodeResult;
    }

    return OperationResult(true, QString("Successfully added %1 ports").arg(portCount));
}

ConfigManager::OperationResult ConfigManager::selectiveAddPorts(const QString& deviceIp,
                                                                const QList<int>& portIndices,
                                                                int startPort, bool ssl,
                                                                bool redundantMode,
                                                                const QString& backupIp)
{
    Q_UNUSED(ssl);
    Logger::instance()->log(Logger::Debug, "ConfigManager", "selectiveAddPorts");

    auto ipResult = InputValidator::validateIpAddress(deviceIp);
    if (!ipResult.first) {
        return OperationResult(false, ipResult.second);
    }

    if (portIndices.isEmpty()) {
        return OperationResult(false, "No ports selected");
    }

    auto portResult = InputValidator::validatePortNumber(startPort);
    if (!portResult.first) {
        return OperationResult(false, portResult.second);
    }

    if (redundantMode && backupIp.isEmpty()) {
        return OperationResult(false, "Backup IP is required for redundant mode");
    }

    if (redundantMode) {
        auto backupIpResult = InputValidator::validateIpAddress(backupIp);
        if (!backupIpResult.first) {
            return OperationResult(false, QString("Invalid backup IP: %1").arg(backupIpResult.second));
        }
    }

    QList<int> sortedPortIndices = portIndices;
    std::sort(sortedPortIndices.begin(), sortedPortIndices.end());
    for (int index : sortedPortIndices) {
        if (index < 1 || index > 16) {
            return OperationResult(false, QString("Invalid port index: %1 (must be 1-16)").arg(index));
        }
    }

    for (int i = 1; i < sortedPortIndices.size(); ++i) {
        if (sortedPortIndices.at(i) != sortedPortIndices.first() + i) {
            return OperationResult(false, "mxaddsvr only supports contiguous port ranges; please select consecutive ports");
        }
    }

    OperationResult backupResult = createBackupBeforeModification();
    if (!backupResult.first) {
        return backupResult;
    }

    QPair<int, int> ports = PortCalculator::calculatePorts(sortedPortIndices.first(), startPort);
    PortModel port;
    port.setIp(deviceIp);
    port.setDataPort(ports.first);
    port.setCmdPort(ports.second);
    port.setInterface("0");
    port.setMode(redundantMode ? 1 : 0);
    if (redundantMode) {
        port.setBackupIp(backupIp);
    }

    OperationResult execResult = executeMxaddsvrForPort(port, sortedPortIndices.size());
    if (!execResult.first) {
        m_config.parseFromFile(m_configFilePath);
        return OperationResult(false, QString("Failed to execute mxaddsvr: %1\nBackup: %2")
                               .arg(execResult.second)
                               .arg(m_lastBackupPath));
    }

    m_config.parseFromFile(m_configFilePath);
    OperationResult nodeResult = verifyDeviceNodes(deviceIp);
    if (!nodeResult.first) {
        return nodeResult;
    }

    return OperationResult(true, QString("Successfully added %1 ports").arg(sortedPortIndices.size()));
}

ConfigManager::OperationResult ConfigManager::deleteDevice(const QString& deviceIp)
{
    ConfigDeviceModel* device = m_config.getDevice(deviceIp);
    if (!device) {
        return OperationResult(false, QString("Device not found: %1").arg(deviceIp));
    }

    OperationResult backupResult = createBackupBeforeModification();
    if (!backupResult.first) {
        return backupResult;
    }

    CommandExecutor::ExecuteResult execResult = CommandExecutor::executeMxdelsvrByIp(deviceIp);
    if (!execResult.first) {
        m_config.parseFromFile(m_configFilePath);
        return OperationResult(false, QString("Failed to execute mxdelsvr: %1\nBackup: %2")
                               .arg(execResult.second)
                               .arg(m_lastBackupPath));
    }

    m_config.parseFromFile(m_configFilePath);

    return OperationResult(true, "Device deleted successfully");
}



const ConfigurationData& ConfigManager::getConfiguration() const
{
    return m_config;
}

QString ConfigManager::getConfigFilePath() const
{
    return m_configFilePath;
}

ConfigManager::OperationResult ConfigManager::createBackupBeforeModification()
{
    if (m_configFilePath.isEmpty()) {
        return OperationResult(false, "Configuration file path is not set");
    }

    QPair<bool, QString> result = BackupManager::createBackup(m_configFilePath);
    if (!result.first)
    {
        return OperationResult(false, result.second);
    }

    m_lastBackupPath = result.second;
    return OperationResult(true, "");
}

void ConfigManager::restoreFromBackup()
{
    if (!m_lastBackupPath.isEmpty()) {
        BackupManager::restoreBackup(m_lastBackupPath, m_configFilePath);
        m_config.parseFromFile(m_configFilePath);
    }
}

ConfigManager::OperationResult ConfigManager::verifyDeviceNodes(const QString& deviceIp) const
{
    const ConfigDeviceModel* device = m_config.getDevice(deviceIp);
    if (!device) {
        return OperationResult(false, QString("Add failed: device %1 was not found in configuration").arg(deviceIp));
    }

    QStringList missingNodes;
    for (const PortModel& port : device->ports()) {
        const QString ttyPath = "/dev/" + port.ttyName();
        if (!QFileInfo::exists(ttyPath)) {
            missingNodes << ttyPath;
        }
    }

    if (!missingNodes.isEmpty()) {
        return OperationResult(false,
                               QString("Add failed: missing device node(s): %1")
                               .arg(missingNodes.join(", ")));
    }

    return OperationResult(true, "");
}

ConfigManager::OperationResult ConfigManager::executeMxaddsvrForPort(const PortModel& port, int portCount)
{
    CommandExecutor::ExecuteResult result = CommandExecutor::executeMxaddsvr(
                port.ip(),
                portCount,
                port.dataPort(),
                port.cmdPort(),
                0,
                0,
                0,
                port.interface(),
                port.mode(),
                port.backupIp()
                );

    if (!result.first)
    {
        return OperationResult(false, result.second);
    }

    return OperationResult(true, "");
}
