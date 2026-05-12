#ifndef COMMANDEXECUTOR_H
#define COMMANDEXECUTOR_H

#include <QString>
#include <QStringList>
#include <QPair>

class CommandExecutor
{
public:
    CommandExecutor();
    
    // 执行命令结果: <成功, 输出或错误信息>
    typedef QPair<bool, QString> ExecuteResult;
    
    // 执行mxaddsvr命令添加端口
    // mxaddsvr [IP] [数据端口] [命令端口] [次设备号] [fifo] [ssl] [接口] [模式] [备份IP]
    static ExecuteResult executeMxaddsvr(const QString& ip, int portCount, int dataPort, int cmdPort,
                                         int minor, int fifo, int ssl,
                                         const QString& interface, int mode,
                                         const QString& backupIp = QString());
    
    // 执行mxdelsvr命令通过IP删除整个设备
    // mxdelsvr [IP]
    static ExecuteResult executeMxdelsvrByIp(const QString& ip);
    
    // 执行mxcfmat命令格式化/初始化配置
    // mxcfmat [tty主设备号] [callout主设备号]
    static ExecuteResult executeMxcfmat(int ttyMajor, int calloutMajor);
    
    // 执行mxrmnod命令删除TTY设备节点
    // mxrmnod [tty名称]
    static ExecuteResult executeMxrmnod(const QString& ttyName);

    // Reload driver service and refresh device nodes from wq_nportd.cf.
    static ExecuteResult executeMxloadsvr();
    
    // 通知守护进程重新加载配置
    // 使用systemctl或service命令
    static ExecuteResult signalDaemonReload();
    
private:
    // 执行命令，必要时提升权限
    static ExecuteResult executeCommand(const QString& command, const QStringList& args,
                                        bool needsPrivilege = true);
    
    // 获取权限提升命令（pkexec或sudo）
    static QString getPrivilegeCommand();
    
    // 检查是否以root权限运行
    static bool hasRootPrivileges();
};

#endif // COMMANDEXECUTOR_H
