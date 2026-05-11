#include "BackupManager.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QStandardPaths>
#include <QProcess>
#include <QStringList>
#include "../../utils/Logger.h"

BackupManager::BackupManager()
{
}

QPair<bool, QString> BackupManager::createBackup(const QString& configFilePath)
{
    if (!QFile::exists(configFilePath))
    {
        return QPair<bool, QString>(false, QString("Configuration file not found: %1").arg(configFilePath));
    }

    QString backupPath = generateBackupPath(configFilePath);
    if (!copyFile(configFilePath, backupPath))
    {
        return QPair<bool, QString>(false, QString("Failed to create backup: %1").arg(backupPath));
    }

    Logger::instance()->log(Logger::Debug, "BackupManager",
                            QString("Backup file created: %1").arg(backupPath));
    return QPair<bool, QString>(true, backupPath);
}

QString BackupManager::getPrivilegeCommand()
{
    QString pkexec = QStandardPaths::findExecutable("pkexec");
    if (!pkexec.isEmpty()) {
        return pkexec;
    }

    QString sudo = QStandardPaths::findExecutable("sudo");
    if (!sudo.isEmpty()) {
        return sudo;
    }

    return QString();
}

QPair<bool, QString> BackupManager::restoreBackup(const QString& backupPath, const QString& configFilePath)
{
    if (!QFile::exists(backupPath)) {
        return QPair<bool, QString>(false, QString("Backup file not found: %1").arg(backupPath));
    }

    QString privilegeCommand = getPrivilegeCommand();
    if (privilegeCommand.isEmpty()) {
        if (copyFile(backupPath, configFilePath)) {
            return QPair<bool, QString>(true, "");
        }
        return QPair<bool, QString>(false, "No privilege escalation tool found for restore.");
    }

    QStringList arguments;
    arguments << "cp" << backupPath << configFilePath;
    QProcess process;
    process.start(privilegeCommand, arguments);
    if (!process.waitForStarted()) {
        return QPair<bool, QString>(false, "Failed to start the restore process.");
    }

    if (!process.waitForFinished(30000)) {
        process.kill();
        return QPair<bool, QString>(false, "Restore operation timed out.");
    }

    if (process.exitCode() != 0) {
        QString errorOutput = process.readAllStandardError();
        return QPair<bool, QString>(false, QString("Restore failed (exit code: %1). Error: %2")
                                    .arg(process.exitCode())
                                    .arg(errorOutput));
    }

    return QPair<bool, QString>(true, "");
}

QString BackupManager::generateBackupPath(const QString& configFilePath)
{
    QString backupDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (backupDir.isEmpty()) {
        backupDir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/.wq_nport";
    }

    backupDir += "/backups";
    QDir().mkpath(backupDir);

    QString fileName = QFileInfo(configFilePath).fileName();
    return QString("%1/%2.last").arg(backupDir, fileName);
}

bool BackupManager::copyFile(const QString& source, const QString& destination)
{
    if (QFile::exists(destination)) {
        if (!QFile::remove(destination)) {
            return false;
        }
    }

    return QFile::copy(source, destination);
}
