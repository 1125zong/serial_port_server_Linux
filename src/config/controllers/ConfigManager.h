#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include "../models/ConfigurationData.h"
#include "../models/PortModel.h"
#include "BackupManager.h"
#include "CommandExecutor.h"
#include <QString>
#include <QList>
#include <QPair>

class ConfigManager
{
public:
    ConfigManager();
    ~ConfigManager();
    
    // 操作结果: <成功, 错误信息>
    typedef QPair<bool, QString> OperationResult;
    
    // 从文件加载配置
    OperationResult loadConfiguration(const QString& configFilePath);
    
    // 保存配置到文件
    OperationResult saveConfiguration(const QString& configFilePath);
    
    // 验证当前配置
    OperationResult validateConfiguration();
    
    // 批量添加设备端口
    // 创建备份，添加端口到配置，执行mxaddsvr命令
    OperationResult batchAddPorts(const QString& deviceIp, int portCount, int startPort,
                                   bool ssl, bool redundantMode, const QString& backupIp);
    
    // 选择性添加设备端口
    // portIndices: 要添加的端口索引列表 (1-16)
    OperationResult selectiveAddPorts(const QString& deviceIp, const QList<int>& portIndices,
                                       int startPort, bool ssl, bool redundantMode,
                                       const QString& backupIp);
    
    // 删除整个设备及其所有端口
    OperationResult deleteDevice(const QString& deviceIp);
    
    // 获取当前配置数据（只读）
    const ConfigurationData& getConfiguration() const;
    
    // 获取配置文件路径
    QString getConfigFilePath() const;
    
private:
    ConfigurationData m_config;
    QString m_configFilePath;
    QString m_lastBackupPath;
    
    // 辅助函数: 修改前创建备份
    OperationResult createBackupBeforeModification();
    
    // 辅助函数: 失败时从备份恢复
    void restoreFromBackup();
    
    // 辅助函数: 为单个端口执行mxaddsvr
    OperationResult executeMxaddsvrForPort(const PortModel& port, int portCount);
    
    // Check that /dev/ttywXX nodes were created after adding a device.
    OperationResult verifyDeviceNodes(const QString& deviceIp) const;
};

#endif // CONFIGMANAGER_H
