#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMessageBox>
#include <QScreen>
#include <QMainWindow>
#include <QFile>
#include <QFileInfo>
#include <QCryptographicHash>   // 如果用 QCryptographicHash::hash
#include <QProgressBar>
#include <QPointer>
#include <QWeakPointer>
#include <QSettings>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include "src/protocol/data.h"
#include "src/protocol/crc.h"
#include "src/utils/customtitlebar.h"
#include "src/controller/DeviceController.h"
#include <QHeaderView>
#include <QList>
#include <QListView>
#include <QStyledItemDelegate>
#include <QButtonGroup>
#include <QRadioButton>
#include <QMenu>
#include <QLabel>
#include <QStringList>
#include <QMetaObject>
#include <QTime>
#include <QElapsedTimer>
#include <QDir>
#include <QProcess>
#include <QTextStream>
#include <QSet>
#include <QMap>
#include <QAction>
#include <QStyle>
#include <QToolBar>
#include <QToolButton>
#include <algorithm>

#if defined(Q_OS_LINUX)
#include <sys/stat.h>
#include <sys/sysmacros.h>
#endif

// 委托类
#include "src/hosting/checkboxcenterdelegate.h"
#include "src/hosting/CenterAlignedDelegate.h"

//2026.01.20新增
#include "src/config/views/ConfigDialog.h"


enum class PendingOperation;
QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void onPortLockStatusReady(const QList<PortLockInfo> &list);
    void startPackageUpgrade(const QString &filePath);
    void startPackageUpgrade(const QByteArray &pkg);
    void parsePackageUpgradeReply(const QByteArray &resp, uint8_t subID);
    void upgradeFinish();
    static QString getDeviceStatusName(QPORT_DeviceState st);
    static QString deviceStateText(quint8 st);

private slots:
    /* ======= UI 触发（保持原有槽名字，信号槽自动关联） ======= */
    void handleFindDeviceTriggered();
    void handleLoginTriggered();
    void handleOverviewTriggered();
    void handleBasicSettingsTriggered();
    void on_Basic_Config_Btn_clicked();
    void on_reset_btn_clicked();
    void on_self_check_btn_clicked();
    void on_use_monitoring_btn_4_clicked();
    void on_lock_btn_clicked();
    void on_unlock_btn_clicked();
    void on_watchdog_btn_clicked();
    void handleFactoryResetTriggered();
    void handleRestartTriggered();
    void on_upgrade_btn_clicked();
    void on_browse_btn_clicked();
    void on_Port_lockout_btn_clicked();
    void on_Port_unlock_btn_clicked();
    void handleBackTriggered();
    void handleCloseListTriggered();
    void handleReturnToSearchPage();
    void handleDriveFindTriggered();
    void handlePortMapTriggered();
    void handlePortSettingsTriggered();
    void on_name_modify_clicked();
    /* ======= DeviceController 回调 ======= */
    void onDeviceDiscovered(const DeviceInfo &device);
    void onSearchCompleted(int count);
    void onDeviceConnected();
    void onDeviceDisconnected();
    void onConnectionError(const QString &err);
    void onOperationSuccess(const QString &op);
    void onOperationFailed(const QString &op, const QString &err);
    void handleDeviceLogTriggered();
    void lockorunlock();
    void handlePortManageTriggered();
    void Self_Check();
    void portLockout();
    void link_info();
    void Status_information();
    void Error_Message();
    void onUpgradeProgress(int ack);         // 进度条前进
    void onUpgradeSuccess();                 // 成功收尾
    void onUpgradeFailed(const QString &err);// 失败收尾
    void showUpgradeMask(int totalPkts);
    void onItemChanged(QTableWidgetItem *item);
    void handleDisconnectTriggered();
    void on_new_name_cursorPositionChanged(int arg1, int arg2);
    void on_chooseAllPortCheckBox_stateChanged(int arg1);
    void updateRuntimeStatus();

private:
    void initTables();
    void initTitleBar();
    void bindDataToUI();
    void appendDeviceLog(const QString &message);
    QString searchTableDeviceText(int row) const;
    QString currentCheckedDeviceText() const;
    QStringList networkConfigChanges(const NetworkConfig &oldCfg, const NetworkConfig &newCfg) const;
    QStringList serialConfigChanges(const SerialPortConfig &oldCfg, const SerialPortConfig &newCfg) const;
    QStringList lockPortNamesFromPayload(const QByteArray &payload) const;
    QList<int> getSelectedRowIndices() const;
    QByteArray collectSelectedLockPorts(int i) const;
    bool validateFirmwarePackage(const QString &filePath, QByteArray &outPkg);
    void setTableRowColor(QTableWidget *table, int row, const QColor &color);
    void setToolBar3Compact(bool compact);
    void initToolButtonObjectNames();
    void initToolBarActionIcons();
    void initStatusBar();
    void setConnectionStatus(bool connected, const QString &ip = QString(), const QString &deviceName = QString());
    void initDeviceLogDock();
    void showFloatingDeviceLogDock();
    void reserveBottomDockSpace();
    void initPortButtons();
    void initFramePortOperationButtons();
    QList<int> selectedPortButtonPorts() const;
    QByteArray portListPayload(const QList<int> &ports) const;
    QByteArray singlePortPayload(int port) const;
    void openPortParamSettings(int port);
    void openPortParamSettings(const QList<int> &ports);
    void openPortModeSettings(int port);
    void openPortModeSettings(const QList<int> &ports);
    void showPortParamDialog(const SerialPortConfig &cfg);
    void showPortModeDialog(const SerialPortMode &modeCfg);
    void lockPortFromButton(int port);
    void lockPortsFromButtons(const QList<int> &ports);
    void unlockPortFromButton(int port);
    void unlockPortsFromButtons(const QList<int> &ports);
    void resetPortFromButton(int port);
    void resetPortsFromButtons(const QList<int> &ports);
    void selfCheckPortFromButton(int port);
    void selfCheckPortsFromButtons(const QList<int> &ports);

private:
    Ui::MainWindow *ui;
    DeviceController *m_devCtl;   // 唯一数据成员
    QProgressBar *m_progress = nullptr;
    QWidget *m_mask = nullptr;
    PendingOperation m_pendingOperation;
    QString m_pendingOperationName;
    QFile m_fwFile;
    UdpDiscovery *m_udpDiscovery = nullptr;
    bool itemsAdded = false;
    bool Resend = false;
    bool m_toolBar3FilterEnabled = true;
    QTimer *m_buttonCoolDownTimer; // 防止搜索按钮重复点击
    SerialPortConfig m_serialPort;
    NetworkConfig m_lastNetworkConfig;
    bool m_hasLastNetworkConfig = false;
    SerialPortConfig m_lastSerialPortConfig;
    bool m_hasLastSerialPortConfig = false;
    int m_pendingParamPort = 0;
    int m_pendingModePort = 0;
    bool m_waitOpenParamDialog = false;
    bool m_waitOpenModeDialog = false;
    QStringList m_pendingLockPortNames;
    QStringList m_pendingUnlockPortNames;
    QString m_pendingDeviceName;
    QString m_PortOldName;
    ConfigDialog* m_configDialog;
    QLabel* m_statusLabel;
    int m_mainWindowBaseMinimumHeight = 0;
    QLabel *m_connDotLabel = nullptr;
    QLabel *m_connStatusLabel = nullptr;
    QLabel *m_deviceNameStatusLabel = nullptr;
    QLabel *m_runtimeStatusLabel = nullptr;
    QTimer *m_runtimeTimer = nullptr;
    QElapsedTimer m_runtimeElapsed;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

};

#endif // MAINWINDOW_H
