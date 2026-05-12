#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include <QDialog>
#include <QTreeWidget>
#include <QToolBar>
#include <QStatusBar>
#include <QVBoxLayout>
#include "../controllers/ConfigManager.h"

class ConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ConfigDialog(QWidget *parent = nullptr);
    ~ConfigDialog();

signals:
    void deviceLogRequested(const QString& message);

private slots:
    // Action slots
    void onRefreshClicked();
    void onBatchAddClicked();
    void onSelectiveAddClicked();
    void onDeleteDeviceClicked();
    void onValidateClicked();
    
    // Tree widget slots
    void onTreeItemSelectionChanged();

private:
    // UI setup
    void setupUi();
    void createToolbar();
    void createTreeWidget();
    void createStatusBar();
    
    // Tree population
    void populateTree();
    void clearTree();
    
    // Helper methods
    void updateActionStates();
    QString getSelectedDeviceIp() const;
    QString buildAddedPortInfo(const QList<int>& portIndices, int startPort) const;
    void showError(const QString& title, const QString& message);
    void showInfo(const QString& title, const QString& message);
    
    // UI components
    QVBoxLayout* m_mainLayout;
    QToolBar* m_toolbar;
    QTreeWidget* m_treeWidget;
    QStatusBar* m_statusBar;
    
    // Actions
    QAction* m_refreshAction;
    QAction* m_batchAddAction;
    QAction* m_selectiveAddAction;
    QAction* m_deleteDeviceAction;
    QAction* m_validateAction;
    
    // Business logic
    ConfigManager* m_configManager;
    
    // Constants
    static const QString CONFIG_FILE_PATH;
    
    // Tree widget columns
    enum TreeColumn {
        COL_NAME = 0,
        COL_IP,
        COL_DATA_PORT,
        COL_CMD_PORT,
        COL_MINOR,
        COL_TTY,
        COL_SSL,
        COL_MODE,
        COL_BACKUP_IP,
        COL_COUNT
    };
};

#endif // CONFIGDIALOG_H
