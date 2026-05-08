#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QByteArray>
#include <QTimer>
#include <QString>
#include <QMessageBox>


/**
 * @brief 增强的TCP客户端
 *
 * 处理TCP粘包/拆包问题,提供完整帧接收功能
 * 实现连接超时和缓冲区溢出保护
 */
class TcpClient : public QObject {
    Q_OBJECT
    
public:
    static bool TcpSocketState;
    QString m_host;
    explicit TcpClient(QObject *parent = nullptr);
    ~TcpClient();
    
    // 连接管理
    void connectToServer(const QString &host, quint16 port);
    bool disconnectFromServer();
    bool disconnectFromUpdateServer();
    bool isConnected() const;
    bool isUpdateConnected() const;
    // 发送数据
    void sendMessage(const QByteArray &data);
    void sendMessageUpdate(const QByteArray &data);
    void firmwareUpdate();  //固件升级
    QString getClientInfo() const;  // 获取客户端信息
    
signals:
    void connected();
    void updateConnected();
    void disconnected();
    void frameReceived(const QByteArray &frame);  // 完整帧
    void errorOccurred(const QString &error);
    void updateBufferHaveData(const bool &isOK);
    
private slots:
    void onConnected();
    void onUpdateConnected();
    void onDisconnected();
    void onReadyRead();
    void onErrorOccurred(QAbstractSocket::SocketError error);
    void onConnectTimeout();
    void onUpdateReadyRead();
private:
    bool tryParseFrame();  // 尝试从缓冲区解析完整帧
    void clearInvalidData();  // 清理无效数据
    int findFrameStart();  // 查找帧头位置
    int findFrameEnd(int startPos);  // 查找帧尾位置
    void cleanup();  // 静态清理方法
    

    QByteArray m_recvBuffer;  // 接收缓冲区
    QByteArray m_UpdateRecvBuffer;
    QTimer *m_connectTimer;   // 连接超时定时器

    quint16 m_port;
    quint16 portUpdate;
    QTcpSocket *m_socket;
    QTcpSocket *updateSocket;

    static constexpr int MAX_BUFFER_SIZE = 65536;  // 最大缓冲区大小
    static constexpr int CONNECT_TIMEOUT = 10000;  // 连接超时(ms)
};

#endif // TCPCLIENT_H
