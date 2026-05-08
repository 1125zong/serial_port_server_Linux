#include "BackupManager.h"
#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <QDir>
#include <QStandardPaths>
#include <QProcess>
#include <QDebug>
#include "../../utils/Logger.h"

BackupManager::BackupManager()
{
}

QPair<bool, QString> BackupManager::createBackup(const QString& configFilePath)
{
    // 检查源文件是否存在
    if (!QFile::exists(configFilePath))
    {
        return QPair<bool, QString>(false, QString("Configuration file not found: %1").arg(configFilePath));
    }
    
    // 生成备份路径
    QString backupPath = generateBackupPath(configFilePath);
    
    // 复制文件
//    if (!copyFile(configFilePath, backupPath))
//    {
//        return QPair<bool, QString>(false, QString("Failed to create backup: %1").arg(backupPath));
//    }


    // 构建复制命令
    QString copyCommand = "cp";
    QStringList arguments;
    arguments << configFilePath << backupPath;

    // 使用 pkexec 或 root 权限执行复制命令
    QProcess process;
    process.setProgram("pkexec");
    process.setArguments(QStringList() << copyCommand << arguments);

    process.start();
    if (!process.waitForStarted()) {
        return QPair<bool, QString>(false, "Failed to start the backup process.");
    }

    // 等待命令执行完成，设置超时时间（例如30秒）
    if (!process.waitForFinished(30000)) {
        process.kill(); // 超时后终止进程
        return QPair<bool, QString>(false, "Backup operation timed out.");
    }

    // 检查执行结果
    if (process.exitCode() != 0) {
        QString errorOutput = process.readAllStandardError();
        return QPair<bool, QString>(false, QString("Backup failed (exit code: %1). Error: %2")
                                    .arg(process.exitCode())
                                    .arg(errorOutput));
    }
    
    Logger::instance()->log(Logger::Debug, "BackupManager", "备份文件创建成功！！！");
    return QPair<bool, QString>(true, backupPath);
}

QString BackupManager::getPrivilegeCommand()
{
    // 检查pkexec是否可用（GUI应用程序首选）
    QString pkexec = QStandardPaths::findExecutable("pkexec");
    if (!pkexec.isEmpty()) {
        return pkexec;
    }

    // 回退到sudo
    QString sudo = QStandardPaths::findExecutable("sudo");
    if (!sudo.isEmpty()) {
        return sudo;
    }

    // 没有可用的权限提升工具
    return QString();
}


QPair<bool, QString> BackupManager::restoreBackup(const QString& backupPath, const QString& configFilePath)
{
    // 检查备份文件是否存在
    if (!QFile::exists(backupPath)) {
        return QPair<bool, QString>(false, QString("Backup file not found: %1").arg(backupPath));
    }
    
    // 将备份复制到配置文件
    if (!copyFile(backupPath, configFilePath)) {
        return QPair<bool, QString>(false, QString("Failed to restore backup from: %1").arg(backupPath));
    }
    
    return QPair<bool, QString>(true, "");
}

QString BackupManager::generateBackupPath(const QString& configFilePath)
{
    // 获取当前时间戳
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    
    // 生成备份路径: original_path.backup.timestamp
    return QString("%1.backup.%2").arg(configFilePath).arg(timestamp);
}

bool BackupManager::copyFile(const QString& source, const QString& destination)
{
    // 如果目标文件存在则删除
    if (QFile::exists(destination)) {
        if (!QFile::remove(destination)) {
            return false;
        }
    }
    
    // 复制文件
    return QFile::copy(source, destination);
}
