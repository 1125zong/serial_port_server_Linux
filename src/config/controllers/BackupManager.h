#ifndef BACKUPMANAGER_H
#define BACKUPMANAGER_H

#include <QString>
#include <QPair>

class BackupManager
{
public:
    BackupManager();
    
    // Create backup of configuration file
    // Returns: <success, backupPath or errorMessage>
    static QPair<bool, QString> createBackup(const QString& configFilePath);
    
    // Restore configuration from backup
    // Returns: <success, errorMessage>
    static QPair<bool, QString> restoreBackup(const QString& backupPath, const QString& configFilePath);
    
    // Generate the fixed "last backup" path in a user-writable app data directory
    // Format: <AppDataLocation>/backups/wq_nportd.cf.last
    static QString generateBackupPath(const QString& configFilePath);

    // Get command to run with sudo or pkexec
    static QString getPrivilegeCommand();
    
private:
    static bool copyFile(const QString& source, const QString& destination);
};

#endif // BACKUPMANAGER_H
