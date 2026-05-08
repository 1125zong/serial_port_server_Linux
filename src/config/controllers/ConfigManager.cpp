#include "ConfigManager.h"
#include "../utils/InputValidator.h"
#include "../utils/PortCalculator.h"
#include "../utils/TtyGenerator.h"
#include "QDebug"
#include "../../utils/Logger.h"

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
    Logger::instance()->log(Logger::Debug, "ConfigManager", QString("saveConfiguration 保存配置到：%1").arg(configFilePath));
    return OperationResult(true, "");
}

ConfigManager::OperationResult ConfigManager::validateConfiguration()
{
    // 检查重复的minor编号
    QSet<int> minors = m_config.usedMinors();
    int totalPorts = m_config.totalPortCount();
    if (minors.size() != totalPorts) {
        return OperationResult(false, "Configuration has duplicate minor numbers");
    }
    
    // 检查重复的TTY名称
    QSet<QString> ttyNames = m_config.usedTtyNames();
    if (ttyNames.size() != totalPorts) {
        return OperationResult(false, "Configuration has duplicate TTY names");
    }
    
    // 验证每个设备
    QList<ConfigDeviceModel> devices = m_config.getDevices();
    for (const ConfigDeviceModel& device : devices) {
        // 验证设备IP
        auto ipResult = InputValidator::validateIpAddress(device.ip());
        if (!ipResult.first) {
            return OperationResult(false, QString("Invalid device IP %1: %2")
                                   .arg(device.ip()).arg(ipResult.second));
        }
        
        // 验证每个端口
        QList<PortModel> ports = device.ports();
        for (const PortModel& port : ports) {
            // 验证minor编号
            auto minorResult = InputValidator::validateMinorNumber(port.minor());
            if (!minorResult.first) {
                return OperationResult(false, QString("Invalid minor number %1: %2")
                                       .arg(port.minor()).arg(minorResult.second));
            }
            
            // 验证数据端口
            auto dataPortResult = InputValidator::validatePortNumber(port.dataPort());
            if (!dataPortResult.first) {
                return OperationResult(false, QString("Invalid data port %1: %2")
                                       .arg(port.dataPort()).arg(dataPortResult.second));
            }
            
            // 验证命令端口
            auto cmdPortResult = InputValidator::validatePortNumber(port.cmdPort());
            if (!cmdPortResult.first) {
                return OperationResult(false, QString("Invalid command port %1: %2")
                                       .arg(port.cmdPort()).arg(cmdPortResult.second));
            }
            
            // 验证TTY名称
            auto ttyResult = InputValidator::validateTtyName(port.ttyName());
            Logger::instance()->log(Logger::Debug, "ConfigManager", QString("port.ttyName(): %1").arg(port.ttyName()));
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
    Logger::instance()->log(Logger::Debug, "ConfigManager", "batchAddPorts：批量添加端口");
    // 验证输入
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
    
    // 创建备份
    OperationResult backupResult = createBackupBeforeModification();
    if (!backupResult.first)
    {
        return backupResult;
    }

    // 如果设备不存在，则创建设备
    ConfigDeviceModel* device = m_config.getDevice(deviceIp);
    if (!device) {
        ConfigDeviceModel newDevice(deviceIp, portCount);
        m_config.addDevice(newDevice);
        device = m_config.getDevice(deviceIp);
    }

    // 增加端口
    QList<PortModel> addedPorts;
    for (int i = 1; i <= portCount; ++i)
    {
        // 计算端口
        QPair<int, int> ports = PortCalculator::calculatePorts(i, startPort);
        int dataPort = ports.first;
        int cmdPort = ports.second;
        
        // 获取下一个可用的 minor and TTY
        int minor = m_config.nextAvailableMinor();
        QString ttyName = m_config.nextAvailableTty();
        QString coutName = TtyGenerator::generateCoutName(ttyName);
        
        // 创建端口模型
        PortModel port;
        port.setMinor(minor);
        port.setIp(deviceIp);
        port.setDataPort(dataPort);
        port.setCmdPort(cmdPort);
        port.setFifo(1);
        port.setSsl(ssl ? 1 : 0);
        port.setTtyName(ttyName);
        port.setCoutName(coutName);
        port.setInterface("0");
        port.setMode(redundantMode ? 1 : 0);
        if (redundantMode) {
            port.setBackupIp(backupIp);
        }
        
        // 添加到设备
        device->addPort(port);
        addedPorts.append(port);
    }

    OperationResult saveResult = saveConfiguration(m_configFilePath);
    if (!saveResult.first)
    {
        restoreFromBackup();
        return OperationResult(false, QString("Failed to save configuration: %1").arg(saveResult.second));
    }
    
    // 为设备执行一次mxaddsvr
    if (!addedPorts.isEmpty()) {
        OperationResult execResult = executeMxaddsvrForPort(addedPorts.first(), portCount);
        if (!execResult.first)
        {
            restoreFromBackup();
            return OperationResult(false, QString("Failed to execute mxaddsvr: %1").arg(execResult.second));
        }
        
        // 重新加载配置文件，确保内存中的配置与实际一致
        m_config.parseFromFile(m_configFilePath);
    }

    // 信号守护进程重新加载
    CommandExecutor::ExecuteResult reloadResult = CommandExecutor::signalDaemonReload();
    if (!reloadResult.first) {
        // 仅警告，不要使操作失败
        return OperationResult(true, QString("Ports added successfully, but daemon reload failed: %1")
                               .arg(reloadResult.second));
    }
    
    return OperationResult(true, QString("Successfully added %1 ports").arg(portCount));
}

ConfigManager::OperationResult ConfigManager::selectiveAddPorts(const QString& deviceIp,
                                                                const QList<int>& portIndices,
                                                                int startPort, bool ssl,
                                                                bool redundantMode,
                                                                const QString& backupIp)
{
    Logger::instance()->log(Logger::Debug, "ConfigManager", "selectiveAddPorts：选择性添加端口");
    // 验证输入
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
    
    // 验证端口索引
    for (int index : portIndices) {
        if (index < 1 || index > 16) {
            return OperationResult(false, QString("Invalid port index: %1 (must be 1-16)").arg(index));
        }
    }
    
    // 创建备份
    OperationResult backupResult = createBackupBeforeModification();
    if (!backupResult.first) {
        return backupResult;
    }
    
    // 从选定的索引推断端口容量
    int maxIndex = *std::max_element(portIndices.begin(), portIndices.end());
    int portCapacity = 1;
    if (maxIndex > 8) portCapacity = 16;
    else if (maxIndex > 4) portCapacity = 8;
    else if (maxIndex > 1) portCapacity = 4;
    
    // 如果设备不存在，则创建设备
    //检查设备是否存在，不存在创建新设备
    ConfigDeviceModel* device = m_config.getDevice(deviceIp);
    if (!device) {
        ConfigDeviceModel newDevice(deviceIp, portCapacity);
        m_config.addDevice(newDevice);
        device = m_config.getDevice(deviceIp);
    }
    
    // 添加选定的端口
    QList<PortModel> addedPorts;
    for (int portIndex : portIndices)
    {
        // 计算端口
        QPair<int, int> ports = PortCalculator::calculatePorts(portIndex, startPort);
        int dataPort = ports.first;
        int cmdPort = ports.second;
        
        // 获取下一个可用的minor和TTY
        int minor = m_config.nextAvailableMinor();  // 得到的是没有使用的minor
        QString ttyName = m_config.nextAvailableTty();
        QString coutName = TtyGenerator::generateCoutName(ttyName);
        
        // 创建端口模型
        PortModel port;
        port.setMinor(minor);
        port.setIp(deviceIp);
        port.setDataPort(dataPort);
        port.setCmdPort(cmdPort);
        port.setFifo(1);
        port.setSsl(ssl ? 1 : 0);
        port.setTtyName(ttyName);
        port.setCoutName(coutName);
        port.setInterface("0");
        port.setMode(redundantMode ? 1 : 0);
        if (redundantMode)
        {
            port.setBackupIp(backupIp);
        }
        
        // 添加到设备
        device->addPort(port);
        addedPorts.append(port);
    }
    
    // 保存配置
    OperationResult saveResult = saveConfiguration(m_configFilePath);
    if (!saveResult.first) {
        restoreFromBackup();
        return OperationResult(false, QString("Failed to save configuration: %1").arg(saveResult.second));
    }
    
    // 为设备执行一次mxaddsvr，添加所有选中的端口
    if (!addedPorts.isEmpty()) {
        OperationResult execResult = executeMxaddsvrForPort(addedPorts.first(), addedPorts.size());
        if (!execResult.first) {
            restoreFromBackup();
            return OperationResult(false, QString("Failed to execute mxaddsvr: %1").arg(execResult.second));
        }
        
        // 重新加载配置文件，确保内存中的配置与实际一致
        m_config.parseFromFile(m_configFilePath);
    }
    Logger::instance()->log(Logger::Debug, "ConfigManager", "为设备执行mxaddsvr添加所有选中的端口");
    // 信号守护进程重新加载
    CommandExecutor::ExecuteResult reloadResult = CommandExecutor::signalDaemonReload();
    if (!reloadResult.first) {
        // Warning only, don't fail the operation
        return OperationResult(true, QString("Ports added successfully, but daemon reload failed: %1")
                               .arg(reloadResult.second));
    }
    
    return OperationResult(true, QString("Successfully added %1 ports").arg(portIndices.size()));
}

ConfigManager::OperationResult ConfigManager::deleteDevice(const QString& deviceIp)
{
    ConfigDeviceModel* device = m_config.getDevice(deviceIp);
    if (!device) {
        return OperationResult(false, QString("设备未找到: %1").arg(deviceIp));
    }
    
    // 创建备份
    OperationResult backupResult = createBackupBeforeModification();
    if (!backupResult.first) {
        return backupResult;
    }
    
    // 直接使用mxdelsvr命令删除整个设备（通过IP）
    CommandExecutor::ExecuteResult execResult = CommandExecutor::executeMxdelsvrByIp(deviceIp);
    if (!execResult.first) {
        // 即使命令执行失败，仍然继续删除配置，因为可能设备已经不存在
        Logger::instance()->log(Logger::Debug, "ConfigManager", QString("执行mxdelsvr命令失败: %1").arg(execResult.second));
    }
    
    // 从配置中删除设备
    m_config.removeDevice(deviceIp);
    
    // 保存配置
    OperationResult saveResult = saveConfiguration(m_configFilePath);
    if (!saveResult.first) {
        restoreFromBackup();
        return OperationResult(false, QString("保存配置失败: %1").arg(saveResult.second));
    }
    
    // 信号守护进程重新加载
    CommandExecutor::ExecuteResult reloadResult = CommandExecutor::signalDaemonReload();
    if (!reloadResult.first) {
        // Warning only, don't fail the operation
        return OperationResult(true, QString("设备删除成功，但守护进程重新加载失败: %1")
                               .arg(reloadResult.second));
    }
    
    return OperationResult(true, QString("设备删除成功"));
}

ConfigManager::OperationResult ConfigManager::deleteSelectedPorts(const QString& deviceIp,
                                                                  const QList<int>& minors)
{
    ConfigDeviceModel* device = m_config.getDevice(deviceIp);
    if (!device) {
        return OperationResult(false, QString("Device not found: %1").arg(deviceIp));
    }
    
    if (minors.isEmpty()) {
        return OperationResult(false, "No ports selected for deletion");
    }
    
    // 创建备份
    OperationResult backupResult = createBackupBeforeModification();
    if (!backupResult.first) {
        return backupResult;
    }
    
    // 为每个端口执行mxdelsvr
    for (int minor : minors) {
        OperationResult execResult = executeMxdelsvrForPort(minor);
        if (!execResult.first) {
            restoreFromBackup();
            return OperationResult(false, QString("Failed to execute mxdelsvr: %1").arg(execResult.second));
        }
    }
    
    // Remove ports from device
    for (int minor : minors) {
        device->removePort(minor);
    }
    
    // If device has no more ports, remove it
    if (device->ports().isEmpty()) {
        m_config.removeDevice(deviceIp);
    }
    
    // 保存配置
    OperationResult saveResult = saveConfiguration(m_configFilePath);
    if (!saveResult.first) {
        restoreFromBackup();
        return OperationResult(false, QString("Failed to save configuration: %1").arg(saveResult.second));
    }
    
    // 信号守护进程重新加载
    CommandExecutor::ExecuteResult reloadResult = CommandExecutor::signalDaemonReload();
    if (!reloadResult.first) {
        // Warning only, don't fail the operation
        return OperationResult(true, QString("Ports deleted successfully, but daemon reload failed: %1")
                               .arg(reloadResult.second));
    }
    
    return OperationResult(true, QString("Successfully deleted %1 ports").arg(minors.size()));
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
        // Reload configuration
        m_config.parseFromFile(m_configFilePath);
    }
}

ConfigManager::OperationResult ConfigManager::executeMxaddsvrForPort(const PortModel& port, int portCount)
{
    Logger::instance()->log(Logger::Debug, "ConfigManager", "selectiveAddPorts333333333333333");
    CommandExecutor::ExecuteResult result = CommandExecutor::executeMxaddsvr(
                port.ip(),
                portCount,
                port.dataPort(),
                port.cmdPort(),
                0, // minor号由mxaddsvr自动分配
                0, // fifo由mxaddsvr默认设置为1
                0, // ssl由mxaddsvr默认设置为0
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

ConfigManager::OperationResult ConfigManager::executeMxdelsvrForPort(int minor)
{
    CommandExecutor::ExecuteResult result = CommandExecutor::executeMxdelsvr(minor);
    
    if (!result.first) {
        return OperationResult(false, result.second);
    }
    
    return OperationResult(true, "");
}
