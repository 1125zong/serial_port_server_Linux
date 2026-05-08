#include "TcpClient.h"
#include "../utils/ProtocolConstants.h"
#include "../utils/Logger.h"
#include <QtEndian>

bool TcpClient::TcpSocketState = false;

TcpClient::TcpClient(QObject *parent)
    : QObject(parent)
    , m_connectTimer(new QTimer(this))
    , m_port(0)
    , m_socket(new QTcpSocket(this))
    , updateSocket(new QTcpSocket(this))
{
    // 连接信号
    connect(m_socket, &QTcpSocket::connected, this, &TcpClient::onConnected);   //成功建立连接发送connected信号
    connect(m_socket, &QTcpSocket::disconnected, this, &TcpClient::onDisconnected);     //断开连接发送disconnected信号
    connect(m_socket, &QTcpSocket::readyRead, this, &TcpClient::onReadyRead);       //有新数据可读发送readyRead信号
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error),     //当发生错误是产生
            this, &TcpClient::onErrorOccurred);

    // 配置连接超时定时器---10s后产生超时信号
    m_connectTimer->setSingleShot(true);    //设置定时器为单次模式。当计数到达时，产生一个timeout信号，并执行一次对应的槽函数。
    m_connectTimer->setInterval(CONNECT_TIMEOUT);
    connect(m_connectTimer, &QTimer::timeout, this, &TcpClient::onConnectTimeout);

    connect(updateSocket, &QTcpSocket::connected, this, &TcpClient::onUpdateConnected);
    connect(updateSocket, &QTcpSocket::readyRead, this, &TcpClient::onUpdateReadyRead);
}

TcpClient::~TcpClient()
{
}

void TcpClient::connectToServer(const QString &host, quint16 port)
{
    Logger::instance()->log(Logger::Debug, "TcpClient", "connectToServer called");

    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        Logger::instance()->log(Logger::Warning, "TcpClient", "已经连接到服务器");
        return;
    }

    m_host = host;
    m_port = port;

    Logger::instance()->log(Logger::Info, "TcpClient",
                            QString("正在连接到 %1:%2").arg(host).arg(port));

    // 清空接收缓冲区---方便下一次连接使用
    m_recvBuffer.clear();

    // 启动连接超时定时器
    m_connectTimer->start();

    // 连接到服务器
    m_socket->connectToHost(host, port);
}

bool TcpClient::disconnectFromServer()
{
    if (m_socket != nullptr && (isConnected() == true))
    {
        Logger::instance()->log(Logger::Info, "TcpClient", QString("正在断开与 %1:%2 的连接").arg(m_host).arg(m_port));
        m_socket->disconnectFromHost();
        // 等待断开完成
        if (m_socket->state() != QAbstractSocket::UnconnectedState)
        {
            Logger::instance()->log(Logger::Debug, "TcpClient", "等待连接断开完成");
            m_socket->waitForDisconnected(1000);       // 如果1秒内连接成功断开，函数返回true；如果超时仍未断开，则返回false
        }
        Logger::instance()->log(Logger::Info, "TcpClient", "连接已断开");
        return true;
    }
    else
    {
        Logger::instance()->log(Logger::Warning, "TcpClient", "未连接，无需断开");
        return false;
    }
}

bool TcpClient::disconnectFromUpdateServer()
{
    qDebug() << "updateSocket->state()：" << updateSocket->state();
    if (updateSocket != nullptr && (updateSocket->state() == QAbstractSocket::ConnectedState))
    {
        Logger::instance()->log(Logger::Info, "TcpClient", "正在断开升级服务器连接");
        updateSocket->disconnectFromHost();
        // 等待断开完成
        if (updateSocket->state() != QAbstractSocket::UnconnectedState)
        {
            Logger::instance()->log(Logger::Debug, "TcpClient", "等待升级连接断开完成");
            updateSocket->waitForDisconnected(1000);       //如果1秒内连接成功断开，函数返回true；如果超时仍未断开，则返回false
        }
        Logger::instance()->log(Logger::Info, "TcpClient", "升级连接已断开");
        return true;
    }
    else
    {
        Logger::instance()->log(Logger::Warning, "TcpClient", "升级连接未建立，无需断开");
        return false;
    }
}

// 实时查询套接字是否处于已连接的状态
bool TcpClient::isConnected() const
{
    return m_socket->state() == QAbstractSocket::ConnectedState;
}
bool TcpClient::isUpdateConnected() const
{
    return updateSocket->state() == QAbstractSocket::ConnectedState;
}


// 发送消息
void TcpClient::sendMessage(const QByteArray &data)
{
    if (!isConnected())
    {
        Logger::instance()->log(Logger::Warning, "TcpClient", "未连接,无法发送数据");
        emit errorOccurred("未连接到服务器");
        return;
    }

    qint64 bytesWritten = m_socket->write(data);
    if (bytesWritten == -1)
    {
        Logger::instance()->log(Logger::Error, "TcpClient", QString("发送数据失败: %1").arg(m_socket->errorString()));
        emit errorOccurred(QString("发送失败: %1").arg(m_socket->errorString()));
    }
    else
    {
        m_socket->flush();
        Logger::instance()->log(Logger::Debug, "TcpClient", QString("发送 %1 字节: %2").arg(bytesWritten).arg(QString(data.toHex(' ').toUpper())));
    }
}

void TcpClient::sendMessageUpdate(const QByteArray &data)
{
    if (!isConnected())
    {
        Logger::instance()->log(Logger::Warning, "TcpClient", "未连接,无法发送数据");
        emit errorOccurred("未连接到服务器");
        return;
    }

    qint64 bytesWritten = updateSocket->write(data);
    if (bytesWritten == -1)
    {
        Logger::instance()->log(Logger::Error, "TcpClient", QString("发送升级数据失败: %1").arg(updateSocket->errorString()));
        emit errorOccurred(QString("发送失败: %1").arg(updateSocket->errorString()));
    }
    else
    {
        updateSocket->flush();
        Logger::instance()->log(Logger::Debug, "TcpClient", QString("发送升级数据 %1 字节: %2").arg(bytesWritten).arg(QString(data.toHex(' ').toUpper())));
    }
}

QString TcpClient::getClientInfo() const
{
    if (isConnected())
    {
        return QString("%1:%2").arg(m_host).arg(m_port);
    }
    return "未连接";
}

void TcpClient::onConnected()
{
    // 停止连接超时定时器
    m_connectTimer->stop();
    Logger::instance()->log(Logger::Info, "TcpClient",
                            QString("已连接到 %1:%2").arg(m_host).arg(m_port));

    emit connected();
}

void TcpClient::onUpdateConnected()
{
    Logger::instance()->log(Logger::Info, "TcpClient",
                            QString("UpdateSocket已连接"));
    emit updateConnected();
}

void TcpClient::onDisconnected()
{
    Logger::instance()->log(Logger::Info, "TcpClient", "连接已断开");
    if (m_socket->state() != QAbstractSocket::UnconnectedState)
    {
        Logger::instance()->log(Logger::Info, "TcpClient", "未断开连接");
    }

    // 停止连接超时定时器
    m_connectTimer->stop();

    // 清空缓冲区
    m_recvBuffer.clear();

    emit disconnected();
}

void TcpClient::onReadyRead() {
    // 读取所有可用数据
    QByteArray newData = m_socket->readAll();

    Logger::instance()->log(Logger::Debug, "TcpClient", QString("接收 %1 字节: %2").arg(newData.size()).arg(QString(newData.toHex(' ').toUpper())));

    // 追加到接收缓冲区
    m_recvBuffer.append(newData);

    // 检查缓冲区溢出
    if (m_recvBuffer.size() > MAX_BUFFER_SIZE)
    {
        Logger::instance()->log(Logger::Error, "TcpClient", QString("接收缓冲区溢出: %1 > %2").arg(m_recvBuffer.size()).arg(MAX_BUFFER_SIZE));
        clearInvalidData();
        emit errorOccurred("接收缓冲区溢出");
        return;
    }

    // 尝试解析完整帧
    while (tryParseFrame())
    {
        // 继续解析,直到没有完整帧
    }
}

// 固件升级，若返回ERROR->0x00, 结束传输
void TcpClient::onUpdateReadyRead()
{
    // 读取所有可用数据
    m_UpdateRecvBuffer = updateSocket->readAll();
    Logger::instance()->log(Logger::Debug, "TcpClient", QString("接收升级数据 %1 字节: %2").arg(m_UpdateRecvBuffer.size()).arg(QString(m_UpdateRecvBuffer.toHex(' ').toUpper())));
    QString value = QString::fromLatin1(m_UpdateRecvBuffer.toHex());
    bool isOK;
    if(value == "00")
    {
        isOK = false;
        emit updateBufferHaveData(isOK);
    }
    else
    {
        isOK = true;
        emit updateBufferHaveData(isOK);
    }
}

void TcpClient::onErrorOccurred(QAbstractSocket::SocketError error) {
    QString errorString = m_socket->errorString();

    Logger::instance()->log(Logger::Error, "TcpClient",
                            QString("Socket错误 [%1]: %2").arg(error).arg(errorString));

    emit errorOccurred(errorString);
}

void TcpClient::onConnectTimeout() {
    if (m_socket->state() != QAbstractSocket::ConnectedState) {
        Logger::instance()->log(Logger::Error, "TcpClient",
                                QString("连接超时: %1:%2").arg(m_host).arg(m_port));

        m_socket->abort();

        emit errorOccurred(QString("连接超时: %1:%2").arg(m_host).arg(m_port));
    }
}

bool TcpClient::tryParseFrame() {
    Logger::instance()->log(Logger::Debug, "TcpClient", QString("[Frame out] %1").arg(QString(m_recvBuffer.toHex(' ').toUpper())));

    // 查找帧头
    int startPos = findFrameStart();
    if (startPos == -1) {
        // 没有找到帧头
        if (m_recvBuffer.size() > 100) {
            // 缓冲区中有太多无效数据,清理
            Logger::instance()->log(Logger::Warning, "TcpClient",
                                    "缓冲区中未找到有效帧头,清理数据");
            m_recvBuffer.clear();
        }
        return false;
    }

    // 如果帧头不在开始位置,丢弃之前的数据
    if (startPos > 0) {
        Logger::instance()->log(Logger::Debug, "TcpClient",
                                QString("丢弃 %1 字节无效数据").arg(startPos));
        m_recvBuffer.remove(0, startPos);
        startPos = 0;
    }

    // 检查是否有足够的数据来确定帧长度
    if (m_recvBuffer.size() < ProtocolConstants::MIN_FRAME_LENGTH) {
        // 数据不足,等待更多数据
        return false;
    }

    // 查找帧尾
    int endPos = findFrameEnd(startPos);
    if (endPos == -1) {
        // 没有找到帧尾,等待更多数据
        return false;
    }

    // 提取完整帧 (包括帧尾的2字节)
    int frameLength = endPos + 2 - startPos;
    QByteArray frame = m_recvBuffer.mid(startPos, frameLength);

    Logger::instance()->log(Logger::Debug, "TcpClient",
                            QString("提取完整帧: %1 字节").arg(frameLength));

    // 从缓冲区移除已处理的帧
    m_recvBuffer.remove(startPos, frameLength);

    // 发出完整帧信号
    emit frameReceived(frame);

    return true;  // 可能还有更多帧
}

void TcpClient::clearInvalidData() {
    Logger::instance()->log(Logger::Warning, "TcpClient", QString("清空接收缓冲区: %1 字节").arg(m_recvBuffer.size()));
    m_recvBuffer.clear();
}

int TcpClient::findFrameStart()
{
    // 查找帧头 0xA5A5 (大端序)
    for (int i = 0; i <= m_recvBuffer.size() - 2; ++i)
    {
        quint16 header = qFromBigEndian<quint16>(reinterpret_cast<const uchar*>(m_recvBuffer.constData() + i));
        if (header == ProtocolConstants::FRAME_HEADER)
        {
            return i;
        }
    }
    return -1;
}

int TcpClient::findFrameEnd(int startPos)
{
    // 从帧头之后开始查找帧尾 0x5A5A (大端序)
    // 至少需要: 帧头(2) + 功能ID(1) + 子功能ID(1) + 帧尾(2) = 6字节
    for (int i = startPos + 4; i <= m_recvBuffer.size() - 2; ++i) {
        quint16 tail = qFromBigEndian<quint16>(
                    reinterpret_cast<const uchar*>(m_recvBuffer.constData() + i));
        if (tail == ProtocolConstants::FRAME_TAIL) {
            return i;
        }
    }
    return -1;
}

void TcpClient::firmwareUpdate()
{
    portUpdate = 19001; //固件升级端口号

    updateSocket->connectToHost(m_host, portUpdate);
}











