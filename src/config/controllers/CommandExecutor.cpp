#include "CommandExecutor.h"
#include <QProcess>
#include <QStandardPaths>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <QDebug>
#include <QFileInfo>
#include "../../utils/Logger.h"

namespace
{
QString shellQuote(const QString& value)
{
    QString quoted = value;
    quoted.replace("'", "'\\''");
    return "'" + quoted + "'";
}
}


CommandExecutor::CommandExecutor()
{

}

CommandExecutor::ExecuteResult CommandExecutor::executeMxaddsvr(const QString& ip, int portCount, int dataPort, int cmdPort,
                                                                int minor, int fifo, int ssl,
                                                                const QString& interface, int mode,
                                                                const QString& backupIp)
{
    const QString driverDir = "/usr/lib/wq_nport/driver";
    QStringList mxaddsvrArgs;

    if(!backupIp.isEmpty())
    {
        // 冗余模式
        mxaddsvrArgs << "-r" << ip << backupIp << QString::number(portCount);
        // 如果指定了数据端口和命令端口，添加到参数中
        if (dataPort > 0 && cmdPort > 0) {
            mxaddsvrArgs << QString::number(dataPort) << QString::number(cmdPort);
        }
    }
    else
    {
        // real COM模式
        mxaddsvrArgs << ip << QString::number(portCount);
        // 如果指定了数据端口和命令端口，添加到参数中
        if (dataPort > 0 && cmdPort > 0) {
            mxaddsvrArgs << QString::number(dataPort) << QString::number(cmdPort);
        }
    }

    QStringList quotedArgs;
    for (const QString& arg : mxaddsvrArgs) {
        quotedArgs << shellQuote(arg);
    }

    const QString script = QString("cd %1 && printf 'y\\n' | ./mxaddsvr %2 && ./mxloadsvr")
            .arg(shellQuote(driverDir))
            .arg(quotedArgs.join(" "));

    QStringList args;
    args << "/bin/sh" << "-c" << script;
    return executeCommand("", args, true);
}

CommandExecutor::ExecuteResult CommandExecutor::executeMxdelsvrByIp(const QString& ip)
{
    QString mxdelsvrPath = "/usr/lib/wq_nport/driver/mxdelsvr";
    QStringList args;
    args << mxdelsvrPath << ip;
    
    return executeCommand("", args, true);
}

CommandExecutor::ExecuteResult CommandExecutor::executeMxcfmat(int ttyMajor, int calloutMajor)
{
    QString mxcfmatPath = "/usr/lib/wq_nport/driver/mxcfmat";
    QStringList args;
    args << mxcfmatPath << QString::number(ttyMajor) << QString::number(calloutMajor);

    return executeCommand("", args, true);
}

CommandExecutor::ExecuteResult CommandExecutor::executeMxrmnod(const QString& ttyName)
{
    QString mxrmnodPath = "/usr/lib/wq_nport/driver/mxrmnod";
    QStringList args;
    args << mxrmnodPath << ttyName;

    return executeCommand("", args, true);
}

CommandExecutor::ExecuteResult CommandExecutor::executeMxloadsvr()
{
    QString mxloadsvrPath = "/usr/lib/wq_nport/driver/mxloadsvr";
    QStringList args;
    args << mxloadsvrPath;

    return executeCommand("", args, true);
}

CommandExecutor::ExecuteResult CommandExecutor::signalDaemonReload()
{
    // 查找 wq_nportd 进程
    QProcess pgrepProcess;
    pgrepProcess.start("/usr/bin/pgrep", QStringList() << "-x" << "wq_nportd");
    if (!pgrepProcess.waitForFinished(5000)) {
        return ExecuteResult(false, "查找 wq_nportd 进程超时");
    }

    QString output = QString::fromUtf8(pgrepProcess.readAllStandardOutput());
    if (pgrepProcess.exitCode() != 0 || output.isEmpty()) {
        return ExecuteResult(false, "wq_nportd 守护进程未运行");
    }

    // 获取进程 ID
    QStringList pids = output.trimmed().split('\n');
    if (pids.isEmpty() || pids.first().isEmpty()) {
        return ExecuteResult(false, "未找到 wq_nportd 进程");
    }

    // 向每个进程发送 SIGUSR1 信号
    for (const QString& pidStr : pids) {
        bool ok;
        int pid = pidStr.toInt(&ok);
        if (!ok) {
            continue;
        }

        if (kill(pid, SIGUSR1) == -1) {
            return ExecuteResult(false, QString("向进程 %1 发送信号失败").arg(pid));
        }
    }

    return ExecuteResult(true, QString("已向 %1 个 wq_nportd 进程发送重载信号").arg(pids.size()));
}

CommandExecutor::ExecuteResult CommandExecutor::executeCommand(const QString& command, const QStringList& args,
                                                               bool needsPrivilege)
{
    Logger::instance()->log(Logger::Debug, "CommandExecutor", QString("CommandExecutor::ExecuteResult CommandExecutor::executeCommand command: %1 args:\n %2").arg(command).arg(args.join(" ")));
    QProcess process;

    // 检查args是否为空
    if (args.isEmpty()) {
        return ExecuteResult(false, "未提供命令参数");
    }

    // 如果command为空，直接执行args[0]作为命令
    if (command.isEmpty()) {
        QString actualCommand = args[0];
        QStringList actualArgs = args.mid(1);
        
        // 设置工作目录为命令所在目录
        QFileInfo cmdInfo(actualCommand);
        QString workingDir = cmdInfo.absolutePath();
        process.setWorkingDirectory(workingDir);
        
        // 传递必要的环境变量
        process.setEnvironment(QProcess::systemEnvironment());
        
        // 如果需要权限且没有root权限，使用权限提升
        if (needsPrivilege && !hasRootPrivileges())
        {
            QString privCmd = getPrivilegeCommand();
            if(!privCmd.isEmpty())
            {
                // 构建权限提升命令和参数
                QStringList privArgs;
                // 对于pkexec，添加--keep-cwd选项保持工作目录
                if (privCmd == "pkexec") {
                    privArgs << "--keep-cwd" << actualCommand;
                } else {
                    privArgs << actualCommand;
                }
                privArgs << actualArgs;
                Logger::instance()->log(Logger::Debug, "CommandExecutor", QString("使用权限提升执行: %1 %2").arg(privCmd).arg(privArgs.join(" ")));
                process.start(privCmd, privArgs);
            } else {
                // 没有权限提升工具，直接执行
                Logger::instance()->log(Logger::Debug, "CommandExecutor", QString("直接执行: %1 %2").arg(actualCommand).arg(actualArgs.join(" ")));
                process.start(actualCommand, actualArgs);
            }
        } else {
            // 不需要权限或已有root权限，直接执行
            Logger::instance()->log(Logger::Debug, "CommandExecutor", QString("直接执行: %1 %2").arg(actualCommand).arg(actualArgs.join(" ")));
            process.start(actualCommand, actualArgs);
        }
    } else {
        // 如果command不为空，直接使用command作为命令，args作为参数
        Logger::instance()->log(Logger::Debug, "CommandExecutor", QString("使用指定命令执行: %1 %2").arg(command).arg(args.join(" ")));
        process.start(command, args);
    }
    
    if (!process.waitForStarted(5000))
    {
        return ExecuteResult(false, QString("启动命令失败: %1").arg(command.isEmpty() ? args[0] : command));
    }

    // Send "y\n" to handle overwrite prompt in mxaddsvr
    QString commandName = command.isEmpty() ? (args.isEmpty() ? "" : args[0]) : command;
    if (commandName.endsWith("mxaddsvr", Qt::CaseInsensitive)) {
        process.write("y\n");
        process.closeWriteChannel();
    }

    if (!process.waitForFinished(30000))    // 30秒超时
    {
        process.kill();
        return ExecuteResult(false, "命令执行超时");
    }
    int exitCode = process.exitCode();
    QString output = QString::fromUtf8(process.readAllStandardOutput());
    QString error = QString::fromUtf8(process.readAllStandardError());

    if (exitCode != 0) {
        QString errorMsg = QString("命令执行失败，退出代码: %1").arg(exitCode);
        if (!error.isEmpty()) {
            errorMsg += QString(": %1").arg(error);
        }
        return ExecuteResult(false, errorMsg);
    }
    return ExecuteResult(true, output);
}

QString CommandExecutor::getPrivilegeCommand()
{
    // 检查pkexec是否可用（GUI应用程序首选）
    QString pkexec = QStandardPaths::findExecutable("pkexec");
    Logger::instance()->log(Logger::Debug, "CommandExecutor", QString("pkexec found: %1, path: %2").arg(!pkexec.isEmpty()).arg(pkexec));
    if (!pkexec.isEmpty()) {
        return pkexec;
    }

    // 回退到sudo
    QString sudo = QStandardPaths::findExecutable("sudo");
    Logger::instance()->log(Logger::Debug, "CommandExecutor", QString("sudo found: %1, path: %2").arg(!sudo.isEmpty()).arg(sudo));
    if (!sudo.isEmpty()) {
        return sudo;
    }

    // 没有可用的权限提升工具
    Logger::instance()->log(Logger::Debug, "CommandExecutor", "No privilege escalation tool found!");
    return QString();
}

bool CommandExecutor::hasRootPrivileges()
{
    // 获取当前进程的有效用户ID，返回0表示root权限
    return (geteuid() == 0);
}
