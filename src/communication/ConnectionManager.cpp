#include "ConnectionManager.h"

ConnectionManager::ConnectionManager(TcpClient *client, QObject *parent)
    : QObject(parent)
    , m_client(client)
    , m_responseTimer(new QTimer(this))
    , m_reconnectTimer(new QTimer(this))
    , m_logger(Logger::instance())
    , m_autoReconnect(true)
    , m_maxReconnectAttempts(DEFAULT_MAX_RECONNECT_ATTEMPTS)
    , m_reconnectInterval(DEFAULT_RECONNECT_INTERVAL)
    , m_responseTimeout(DEFAULT_RESPONSE_TIMEOUT)
    , m_currentAttempt(0)
    , m_lastPort(0)
    , m_waitingForResponse(false)
{
    // 配置响应超时定时器
    m_responseTimer->setSingleShot(true);
    connect(m_responseTimer, &QTimer::timeout, this, &ConnectionManager::onResponseTimeout);

    // 配置重连定时器
    m_reconnectTimer->setSingleShot(true);
    connect(m_reconnectTimer, &QTimer::timeout, this, &ConnectionManager::attemptReconnect);

    // 连接TCP客户端信号
    connect(m_client, &TcpClient::connected, this, &ConnectionManager::onConnected);
    connect(m_client, &TcpClient::disconnected, this, &ConnectionManager::onDisconnected);
    connect(m_client, &TcpClient::frameReceived, this, &ConnectionManager::onResponseReceived);

    m_logger->log(Logger::Info, "ConnectionManager", "Connection Manager initialized");
}

ConnectionManager::~ConnectionManager()
{
    m_responseTimer->stop();
    m_reconnectTimer->stop();
    m_logger->log(Logger::Info, "ConnectionManager", "Connection Manager destroyed");
}

void ConnectionManager::setAutoReconnect(bool enable)
{
    m_autoReconnect = enable;
    m_logger->log(Logger::Info, "ConnectionManager",
                  QString("Auto-reconnect %1").arg(enable ? "enabled" : "disabled"));
}

void ConnectionManager::setMaxReconnectAttempts(int max)
{
    m_maxReconnectAttempts = max;
    m_logger->log(Logger::Info, "ConnectionManager",
                  QString("Max reconnect attempts set to %1").arg(max));
}

void ConnectionManager::setReconnectInterval(int ms)
{
    m_reconnectInterval = ms;
    m_logger->log(Logger::Info, "ConnectionManager",
                  QString("Reconnect interval set to %1 ms").arg(ms));
}

void ConnectionManager::setResponseTimeout(int ms)
{
    m_responseTimeout = ms;
    m_logger->log(Logger::Info, "ConnectionManager",
                  QString("Response timeout set to %1 ms").arg(ms));
}

void ConnectionManager::setConnectionInfo(const QString &host, quint16 port)
{
    m_lastHost = host;
    m_lastPort = port;
    m_logger->log(Logger::Info, "ConnectionManager",
                  QString("Connection info set: %1:%2").arg(host).arg(port));
}

void ConnectionManager::sendCommand(const QByteArray &cmd, int timeoutMs)
{
    if (!m_client->isConnected())
    {
        QString error = "Cannot send command: not connected";
        m_logger->log(Logger::Error, "ConnectionManager", error);
        emit commandFailed(error);
        return;
    }

    // 如果当前正在等待响应，将命令加入队列
    if (m_waitingForResponse)
    {
        m_commandQueue.enqueue(cmd);
        m_logger->log(Logger::Info, "ConnectionManager", QString("Command queued (%1 bytes), queue size: %2").arg(cmd.size()).arg(m_commandQueue.size()));
        return;
    }

    // 保存命令用于可能的重发
    m_lastCommand = cmd;
    // 发送命令
    m_client->sendMessage(cmd);
    m_logger->log(Logger::Debug, "ConnectionManager", QString("Command sent (%1 bytes): %2").arg(cmd.size()).arg(QString(cmd.toHex(' ').toUpper())));

    // 启动响应超时定时器
    int timeout = (timeoutMs > 0) ? timeoutMs : m_responseTimeout;
    startResponseTimer(timeout);
}

void ConnectionManager::processNextCommand()
{
    if (m_commandQueue.isEmpty() || !m_client->isConnected())
        return;

    QByteArray cmd = m_commandQueue.dequeue();
    m_logger->log(Logger::Info, "ConnectionManager", QString("Processing next command from queue (%1 bytes), remaining: %2").arg(cmd.size()).arg(m_commandQueue.size()));

    // 保存命令用于可能的重发
    m_lastCommand = cmd;
    // 发送命令
    m_client->sendMessage(cmd);
    m_logger->log(Logger::Debug, "ConnectionManager", QString("Command sent (%1 bytes): %2").arg(cmd.size()).arg(QString(cmd.toHex(' ').toUpper())));

    // 启动响应超时定时器
    startResponseTimer(m_responseTimeout);
}

// 固件升级创建的TCP连接---因为固件升级过程中只有发生错误才会返回消息，不需要响应超时
void ConnectionManager::sendCommandUpdate(const QByteArray &cmd)
{
    if (!m_client->isConnected())
    {
        QString error = "Cannot send command: not connected";
        m_logger->log(Logger::Error, "ConnectionManager", error);
        emit commandFailed(error);
        return;
    }

    // 保存命令用于可能的重发
    m_lastCommand = cmd;
    // 发送命令
    m_client->sendMessageUpdate(cmd);
    m_logger->log(Logger::Debug, "ConnectionManager", QString("Command sent (update) (%1 bytes): %2").arg(cmd.size()).arg(QString(cmd.toHex(' ').toUpper())));
}

void ConnectionManager::onConnected()
{
    // 连接成功，重置重连状态
    resetReconnectState();
    
    m_logger->log(Logger::Info, "ConnectionManager", "Connection established");
    
    // 如果有待发送的命令，重新发送
    if (!m_lastCommand.isEmpty()) {
        m_logger->log(Logger::Info, "ConnectionManager", "Resending last command after reconnect");
        sendCommand(m_lastCommand);
    }
}

void ConnectionManager::onDisconnected()
{
    m_logger->log(Logger::Warning, "ConnectionManager", "Connection lost");

    // 停止响应定时器
    stopResponseTimer();

    // 如果启用了自动重连
    // if (m_autoReconnect && !m_lastHost.isEmpty()) {
    //     resetReconnectState();
    //     m_logger->log(Logger::Info, "ConnectionManager",
    //                   QString("Starting auto-reconnect (max attempts: %1)").arg(m_maxReconnectAttempts));
        
    //     // 延迟后开始第一次重连尝试
    //     m_reconnectTimer->start(m_reconnectInterval);
    // }
}

void ConnectionManager::onResponseReceived(const QByteArray &frame)
{
    if (m_waitingForResponse) {
        // 停止响应超时定时器
        stopResponseTimer();

        // 重置重连状态
        resetReconnectState();

        m_logger->log(Logger::Debug, "ConnectionManager",
                      QString("Response received (%1 bytes): %2").arg(frame.size()).arg(QString(frame.toHex(' ').toUpper())));
        
        emit commandSuccess(frame);

        // 处理队列中的下一个命令
        processNextCommand();
    }
}

void ConnectionManager::onResponseTimeout()
{
    m_waitingForResponse = false;
    m_logger->log(Logger::Warning, "ConnectionManager",
                  QString("Response timeout after %1 ms").arg(m_responseTimeout));
    
    emit commandTimeout();

    // 处理队列中的下一个命令
    processNextCommand();
}

void ConnectionManager::attemptReconnect()
{
    if (m_currentAttempt >= m_maxReconnectAttempts) {
        m_logger->log(Logger::Error, "ConnectionManager",
                      QString("Reconnect failed after %1 attempts").arg(m_currentAttempt));
        resetReconnectState();
        emit reconnectFailed();
        return;
    }

    m_currentAttempt++;
    m_logger->log(Logger::Info, "ConnectionManager",
                  QString("Reconnect attempt %1/%2 to %3:%4")
                  .arg(m_currentAttempt)
                  .arg(m_maxReconnectAttempts)
                  .arg(m_lastHost)
                  .arg(m_lastPort));

    emit reconnecting(m_currentAttempt);

    // 尝试重新连接
    m_client->connectToServer(m_lastHost, m_lastPort);
}

void ConnectionManager::resetReconnectState()
{
    m_currentAttempt = 0;
    m_reconnectTimer->stop();
}

void ConnectionManager::startResponseTimer(int timeoutMs)
{
    m_waitingForResponse = true;
    m_responseTimer->start(timeoutMs);
    m_logger->log(Logger::Debug, "ConnectionManager",
                  QString("Response timer started (%1 ms)").arg(timeoutMs));
}

void ConnectionManager::stopResponseTimer()
{
    m_waitingForResponse = false;
    m_responseTimer->stop();
}
