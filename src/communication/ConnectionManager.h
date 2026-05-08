#ifndef CONNECTIONMANAGER_H
#define CONNECTIONMANAGER_H

#include <QObject>
#include <QTimer>
#include <QString>
#include <QByteArray>
#include <QQueue>
#include "TcpClient.h"
#include "../utils/Logger.h"

/**
 * @brief 连接管理器
 * 
 * 负责管理TCP连接的自动重连和响应超时处理
 * 实现Requirements 9.1-9.5中的连接管理功能
 */
class ConnectionManager : public QObject {
    Q_OBJECT

public:
    explicit ConnectionManager(TcpClient *client, QObject *parent = nullptr);
    ~ConnectionManager();

    /**
     * @brief 设置是否启用自动重连
     */
    void setAutoReconnect(bool enable);

    /**
     * @brief 设置最大重连尝试次数
     */
    void setMaxReconnectAttempts(int max);

    /**
     * @brief 设置重连间隔(毫秒)
     */
    void setReconnectInterval(int ms);

    /**
     * @brief 设置响应超时时间(毫秒)
     */
    void setResponseTimeout(int ms);

    /**
     * @brief 设置连接信息（用于自动重连）
     * @param host 主机地址
     * @param port 端口号
     */
    void setConnectionInfo(const QString &host, quint16 port);

    /**
     * @brief 发送命令并等待响应
     * @param cmd 要发送的命令
     * @param timeoutMs 超时时间(毫秒)，-1表示使用默认超时
     */
    void sendCommand(const QByteArray &cmd, int timeoutMs = -1);
    void sendCommandUpdate(const QByteArray &cmd);
    void processNextCommand();

    /**
     * @brief 获取当前重连尝试次数
     */
    int currentReconnectAttempt() const { return m_currentAttempt; }

    /**
     * @brief 检查是否正在等待响应
     */
    bool isWaitingForResponse() const { return m_responseTimer->isActive(); }

signals:
    /**
     * @brief 命令成功信号
     * @param response 接收到的响应数据
     */
    void commandSuccess(const QByteArray &response);

    /**
     * @brief 命令超时信号
     */
    void commandTimeout();

    /**
     * @brief 命令失败信号
     * @param error 错误描述
     */
    void commandFailed(const QString &error);

    /**
     * @brief 正在重连信号
     * @param attempt 当前重连尝试次数
     */
    void reconnecting(int attempt);

    /**
     * @brief 重连失败信号
     */
    void reconnectFailed();

private slots:
    void onConnected();
    void onDisconnected();
    void onResponseReceived(const QByteArray &frame);
    void onResponseTimeout();
    void attemptReconnect();

private:
    void resetReconnectState();
    void startResponseTimer(int timeoutMs);
    void stopResponseTimer();

    TcpClient *m_client;
    QTimer *m_responseTimer;
    QTimer *m_reconnectTimer;
    Logger *m_logger;

    bool m_autoReconnect;
    int m_maxReconnectAttempts;
    int m_reconnectInterval;
    int m_responseTimeout;
    int m_currentAttempt;

    QString m_lastHost;
    quint16 m_lastPort;
    QByteArray m_lastCommand;
    bool m_waitingForResponse;
    QQueue<QByteArray> m_commandQueue;

    static constexpr int DEFAULT_RESPONSE_TIMEOUT = 5000;  // 默认响应超时(ms)
    static constexpr int DEFAULT_RECONNECT_INTERVAL = 3000;  // 默认重连间隔(ms)
    static constexpr int DEFAULT_MAX_RECONNECT_ATTEMPTS = 5;  // 默认最大重连次数
};

#endif // CONNECTIONMANAGER_H
