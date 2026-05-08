#include "UdpDiscovery.h"
#include "../utils/Utils.h"
#include "../utils/ProtocolConstants.h"
#include <QNetworkDatagram>

DeviceInfo UdpDiscovery::dev;
UdpDiscovery* UdpDiscovery::m_instance = nullptr;

UdpDiscovery::UdpDiscovery(QObject *parent)
    : QObject(parent)
    , m_socket(new QUdpSocket(this))
    , m_searchTimer(new QTimer(this))
    , m_logger(Logger::instance())
    , m_deviceCount(0)
    , m_isSearching(false)
{

    // 配置搜索超时定时器
    // 5s后仅发送一次timeout信号
    m_searchTimer->setSingleShot(true); // 设置定时器为单次触发模式
    m_searchTimer->setInterval(SEARCH_TIMEOUT);

    // 判断连接是否超时
    connect(m_searchTimer, &QTimer::timeout, this, &UdpDiscovery::onSearchTimeout);

    // 连接UDP socket信号
    connect(m_socket, &QUdpSocket::readyRead, this, &UdpDiscovery::onReadyRead);

    m_logger->log(Logger::Info, "UdpDiscovery", "UDP Discovery initialized");
}

UdpDiscovery* UdpDiscovery::instance(QObject *parent)
{
    if (!m_instance) {
        m_instance = new UdpDiscovery(parent);
    }
    return m_instance;
}

UdpDiscovery::~UdpDiscovery()
{
    stopSearch();
    m_logger->log(Logger::Info, "UdpDiscovery", "UDP Discovery destroyed");
}


// 通过UDP套接字以广播的形式向port为48899，ip为192.168.5.x网段的主机发送UDP_SEARCH_MSG的消息，并启动 搜索超时定时器。
void UdpDiscovery::startSearch()
{
    if (m_isSearching) {
        m_logger->log(Logger::Warning, "UdpDiscovery", "Search already in progress");
        return;
    }

    m_deviceCount = 0;
    m_isSearching = true;

    // 如果之前已经绑定过，先关闭，避免重复 bind 异常
    if (m_socket->state() != QAbstractSocket::UnconnectedState)
    {
        m_socket->close();
    }

    // 如果你加了设备去重集合，这里也要清空
    // m_foundDeviceKeys.clear();

    // 1. 绑定端口
    if (!m_socket->bind(QHostAddress::Any, UDP_PORT, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint))
    {
        QString error = QString("Failed to bind UDP port %1: %2").arg(UDP_PORT).arg(m_socket->errorString());
        m_logger->log(Logger::Error, "UdpDiscovery", error);
        m_isSearching = false;
        emit searchError(error);
        return;
    }

    m_logger->log(Logger::Info, "UdpDiscovery", QString("成功绑定UDP端口 %1").arg(UDP_PORT));
    emit bindSucceeded(UDP_PORT);

    QByteArray searchMsg(ProtocolConstants::UDP_SEARCH_MSG);

    QStringList broadcastAddresses;
    QSet<QString> broadcastAddressSet;

    foreach (const QNetworkInterface &interface, QNetworkInterface::allInterfaces())
    {

        QString ifName = interface.humanReadableName();

        // 过滤未运行网卡
        if (!(interface.flags() & QNetworkInterface::IsRunning))
            continue;

        // 过滤回环网卡
        if (interface.flags() & QNetworkInterface::IsLoopBack)
            continue;

        // 过滤虚拟网卡
        if (ifName.contains("VMware", Qt::CaseInsensitive) || ifName.contains("Virtual", Qt::CaseInsensitive) || ifName.contains("VirtualBox", Qt::CaseInsensitive) || ifName.contains("Loopback", Qt::CaseInsensitive))
        {

            m_logger->log(Logger::Debug, "UdpDiscovery",
                          QString("跳过虚拟网卡: %1").arg(ifName));
            continue;
        }

        // 如果你的串口服务器只接有线网卡，可以先跳过无线网卡
        if (ifName.contains("WLAN", Qt::CaseInsensitive) || ifName.contains("Wi-Fi", Qt::CaseInsensitive) || ifName.contains("无线", Qt::CaseInsensitive))
        {

            m_logger->log(Logger::Debug, "UdpDiscovery", QString("跳过无线网卡: %1").arg(ifName));
            continue;
        }

        foreach (const QNetworkAddressEntry &entry, interface.addressEntries())
        {

            QHostAddress ipAddr = entry.ip();
            QHostAddress broadcastAddr = entry.broadcast();

            // 只处理 IPv4
            if (ipAddr.protocol() != QAbstractSocket::IPv4Protocol)
                continue;

            if (broadcastAddr.isNull())
                continue;

            QString addr = broadcastAddr.toString();

            // 广播地址去重
            if (broadcastAddressSet.contains(addr)) {
                m_logger->log(Logger::Info, "UdpDiscovery", QString("广播地址重复，已跳过: %1，来源网卡: %2").arg(addr).arg(ifName));
                continue;
            }

            broadcastAddressSet.insert(addr);
            broadcastAddresses << addr;

            m_logger->log(Logger::Info, "UdpDiscovery", QString("在网卡 %1 上找到广播地址: %2").arg(ifName).arg(addr));
        }
    }

    if (broadcastAddresses.isEmpty()) {
        m_logger->log(Logger::Warning, "UdpDiscovery",
                      "未能获取到网卡广播地址，将使用默认全局广播地址 255.255.255.255");

        broadcastAddresses << "255.255.255.255";
    }

    for (const QString &addr : broadcastAddresses) {
        qint64 sent = m_socket->writeDatagram(searchMsg,
                                              QHostAddress(addr),
                                              UDP_PORT);

        if (sent == -1) {
            QString error = QString("向广播地址 %1 发送失败: %2")
                                .arg(addr)
                                .arg(m_socket->errorString());

            m_logger->log(Logger::Error, "UdpDiscovery", error);
            continue;
        }

        m_logger->log(Logger::Info, "UdpDiscovery",
                      QString("向广播地址 %1 发送成功 (%2 bytes)")
                          .arg(addr)
                          .arg(sent));
    }

    m_searchTimer->start();
}

void UdpDiscovery::stopSearch()
{
    if (!m_isSearching) {
        return;
    }

    m_searchTimer->stop();
    m_socket->close();
    m_isSearching = false;

    m_logger->log(Logger::Info, "UdpDiscovery", "Search stopped");
}

void UdpDiscovery::onReadyRead()
{
    static QList<QHostAddress> localAddrs = getLocalAddresses(); // 辅助函数获取本机所有IP

    //一直判断UDP套接字的缓冲区中是否有数据
    while (m_socket->hasPendingDatagrams())
    {
        //有的话，执行以下操作
        qint64 size = m_socket->pendingDatagramSize();  //该方法精准获取下一个数据报的大小
        QByteArray dg(size, Qt::Uninitialized); // 分配指定大小的内存,并不对内存进行初始化操作
        QHostAddress sender;
        quint16 senderPort;

        // 读取数据
        //数据保存在dg中
        qint64 bytesRead = m_socket->readDatagram(dg.data(), dg.size(), &sender, &senderPort);
        if (bytesRead != size) {
            m_logger->log(Logger::Warning, "UdpDiscovery", QString("读取数据不完整，期望 %1 字节，实际读取 %2 字节").arg(size).arg(bytesRead));
            continue;
        }

        QString hexStr = QString(dg.toHex(' ')).toUpper();  // 将二进制数据转换为大写十六进制字符串，并用空格分隔
        // QString hexStr = "51 70 6F 72 74 31 32 35 35 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 07 E9 05 01 00 00 00 01 51 70 6F 72 74 31 32 35 35 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 01 00 00 01 01 02 C0 A8 05 DC 00 0E C6 01 02 03 01 C0 A8 09 DE 00 0E C6 01 02 04 01 01 01 00 00 00 01 10 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 C2 01 00 08 01 00 00 00 00 01 00 00 01 00 C2 01 00 08 01 00 00 00 00 01 00 00 02 00 C2 01 00 08 01 00 00 00 00 01 00 00 03 00 C2 01 00 08 01 00 00 00 00 01 00 00 04 00 C2 01 00 08 01 00 00 00 00 01 00 00 05 00 C2 01 00 08 01 00 00 00 00 01 00 00 06 00 C2 01 00 08 01 00 00 00 00 01 00 00 07 00 C2 01 00 08 01 00 00 00 00 01 00 00 08 00 C2 01 00 08 01 00 00 00 00 01 00 00 09 00 C2 01 00 08 01 00 00 00 00 01 00 00 0A 00 C2 01 00 08 01 00 00 00 00 01 00 00 0B 00 C2 01 00 08 01 00 00 00 00 01 00 00 0C 00 C2 01 00 08 01 00 00 00 00 01 00 00 0D 00 C2 01 00 08 01 00 00 00 00 01 00 00 0E 00 C2 01 00 08 01 00 00 00 00 01 00 00 0F 00 C2 01 00 08 01 00 00 00 00 01 00 00";

        // 移除空格并转为QByteArray
        hexStr = hexStr.replace(" ", "");
        dg = QByteArray::fromHex(hexStr.toUtf8());  //toUtf8()将QString类型转换为QByteArray类型（存储的是字符的 ASCII 码），fromHex()将进一步转换为机器可识别的原始二进制流
        Logger::instance()->log(Logger::Info, "UdpDiscovery", QString("模拟UDP数据长度：%1").arg(QString(dg.toHex(' ')))); // 打印数据长度，验证转换成功
        /* 长度守门 -------------------------------------------------------*/
        if (dg.size() < 41)
        {
            m_logger->log(Logger::Warning, "UdpDiscovery", "UDP packet too short");
            continue;
        }


        /* 解析 -----------------------------------------------------------*/
        if (parseDiscoveryResponseBinary(dg, sender, UdpDiscovery::dev))
        {
            m_deviceCount++;
            emit deviceDiscovered(UdpDiscovery::dev);
        }
    }
}

void UdpDiscovery::onSearchTimeout()
{
    m_logger->log(Logger::Info, "UdpDiscovery",
                  QString("Search timeout, found %1 device(s)").arg(m_deviceCount));

    stopSearch();
    emit searchCompleted(m_deviceCount);
}

bool UdpDiscovery::parseDiscoveryResponse(const QByteArray &data, const QHostAddress &sender, DeviceInfo &device)
{
    // 根据协议11.1，UDP发现响应可能是分号分隔的ASCII字符串或二进制格式
    // 这里实现ASCII格式解析（更常见）
    // 格式示例: "QPORT;1255xyzp;25125001;192.168.1.100;00:1A:2B:3C:4D:5E"

    QString response = QString::fromUtf8(data).trimmed();

    // 检查是否以"QPORT"开头
    if (!response.startsWith("QPORT")) {
        m_logger->log(Logger::Debug, "UdpDiscovery", "Response does not start with QPORT");
        return false;
    }

    // 分割字段
    QStringList fields = response.split(';');
    if (fields.size() < 5) {
        m_logger->log(Logger::Warning, "UdpDiscovery",
                      QString("Invalid response format, expected at least 5 fields, got %1").arg(fields.size()));
        return false;
    }

    // 解析字段
    device.deviceModel = fields[1].trimmed();
    device.serialNum = fields[2].trimmed();
    device.deviceName = fields[2].trimmed();  // 默认使用序列号作为设备名
    device.lan1Ip = fields[3].trimmed();
    device.lan1Mac = fields[4].trimmed();

    // 如果响应中没有IP地址，使用发送者地址
    if (device.lan1Ip.isEmpty() || device.lan1Ip == "0.0.0.0") {
        device.lan1Ip = sender.toString();
    }

    // 设置默认值
    device.deviceState = QPORT_DeviceState::WorkingMode;
    device.validLanCount = 1;
    device.lan1LinkState = QPORT_LanLinkState::Linked;
    device.devLockState = QPORT_DeviceLockState::Unlocked;

    // 验证必需字段
    if (device.deviceModel.isEmpty() || device.serialNum.isEmpty() || device.lan1Ip.isEmpty()) {
        m_logger->log(Logger::Warning, "UdpDiscovery", "Missing required fields in response");
        return false;
    }

    return true;
}


//就是将dg中的数据进行分割转换到decice中
bool UdpDiscovery::parseDiscoveryResponseBinary(const QByteArray &dg, const QHostAddress &sender, DeviceInfo &device)
{
    /* 1. ASCII 字段 ----------------------------------------------------*/
    //fromLatin1()将原始二进制流转换为QString类型的字符串,trimmed()去除转换后的首端和尾端的无效字符（如"Qport1255\0\0\0\0..."），得到干净的有效字符串（"Qport1255"）
    device.deviceModel = QString::fromLatin1(dg.mid(0, 40)).trimmed();   // 0-39
    device.deviceName  = QString::fromLatin1(dg.mid(48, 40)).trimmed();  // 48-87

    /* 序列号：8 字节 ASCII → 16 进制字符串 yyyyMMddnnn */
    QByteArray snRaw = dg.mid(40, 8);
    QString raw;
    //将snRaw的值转换为无符号char,最小宽度为2，不足补0的16进制的大写QString类型的数据
    for (unsigned char c : snRaw)
        raw += QString("%1").arg(c, 2, 16, QChar('0')).toUpper();

    QString yyyy = raw.mid(0, 4);
    unsigned short yearVal = yyyy.toUShort(nullptr, 16);
    QString year = QString::number(yearVal);  // 年份一般4位，不需要补零

    QString MM = raw.mid(4, 2);
    unsigned short monthVal = MM.toUShort(nullptr, 16);
    QString mm = QString("%1").arg(monthVal, 2, 10, QChar('0'));  // 月份：补零为两位

    QString dd = raw.mid(6, 2);
    unsigned short dayVal = dd.toUShort(nullptr, 16);
    QString DD = QString("%1").arg(dayVal, 2, 10, QChar('0'));  // 日期：补零为两位

    QString nnn = raw.mid(8, 8);
    unsigned short seqVal = nnn.toUShort(nullptr, 16);
    QString NNN = QString("%1").arg(seqVal, 4, 10, QChar('0'));  // 序号：补零为四位

    // 拼接结果：例如 "202505010001"
    QString datastr = QString("%1%2%3%4").arg(year, mm, DD, NNN);
    device.serialNum = datastr;

    /* 2. 固件版本 4B 小端 → 8位 16 进制字符串 */
    // 修复类型转换错误，提取固件版本
    quint32 fwRaw = qFromLittleEndian<quint32>(reinterpret_cast<const uchar*>(dg.mid(88, 4).constData()));

    // 将固件版本转换为 V1.001 格式
    quint8 mainVer = (fwRaw >> 24) & 0xFF;  // 主版本（第1字节）
    quint16 subVer = fwRaw & 0x00FFFFFF;    // 次版本（后3字节）
    device.firmwareVer = QString("V%1.%2").arg(mainVer).arg(subVer, 3, 10, QChar('0'));

    /* 3. 单字节枚举 / 数值 ---------------------------------------------*/
    device.deviceState   = static_cast<QPORT_DeviceState>(static_cast<quint8>(dg[92]));
    device.validLanCount = static_cast<quint8>(dg[93]);


    /* 4. IP 地址（大端） -----------------------------------------------*/
    auto ipFromLE = [&dg](int off) -> QString {
        quint32 ip = qFromLittleEndian<quint32>(
                    reinterpret_cast<const uchar*>(dg.constData() + off));
        return QHostAddress(ip).toString();
    };
    device.lan1Ip = ipFromLE(94);
    device.lan2Ip = ipFromLE(105);

    /* 5. MAC 地址（6 字节原序） ----------------------------------------*/
    auto macToStr = [&dg](int off) -> QString {
        QStringList parts;
        for (int i = 0; i < 6; ++i) {
            quint8 b = static_cast<quint8>(dg[off + i]);
            parts << QString("%1").arg(b, 2, 16, QChar('0')).toUpper();
        }
        return parts.join(":");
    };
    device.lan1Mac = macToStr(98);  //98~103
    device.lan2Mac = macToStr(109); //109~114

    /* 6. 其他单字节 ----------------------------------------------------*/
    device.lcdState     = static_cast<QPORT_LCDState>(static_cast<quint8>(dg[116]));
    device.power1State  = static_cast<QPORT_PowerState>(static_cast<quint8>(dg[117]));
    device.power2State  = static_cast<QPORT_PowerState>(static_cast<quint8>(dg[118]));
    device.devLockState = static_cast<QPORT_DeviceLockState>(static_cast<quint8>(dg[139]));

    /* 7. 端口链接状态（协议4.2字段16：1Byte，枚举值） --------------------*/
    device.lan1LinkState = static_cast<QPORT_LanLinkState>(static_cast<quint8>(dg[104]));
    device.lan2LinkState = static_cast<QPORT_LanLinkState>(static_cast<quint8>(dg[115]));

    device.ifTypeCount = QString::fromLatin1(dg.mid(119, 4)).trimmed();
    for (int i = 0; i < 16; ++i) device.ifStatus[i]     = static_cast<quint8>(dg[123 + i]);
    for (int i = 0; i < 16; ++i) device.ifLockStatus[i] = static_cast<quint8>(dg[140 + i]);

    /* 10. 日志 ----------------------------------------------------------*/
    m_logger->log(Logger::Info, "UdpDiscovery",
                  QString("Parsed %1 (%2) at %3")
                  .arg(device.deviceModel)
                  .arg(device.serialNum)
                  .arg(device.lan1Ip));
    return true;
}

QList<QHostAddress> UdpDiscovery::getLocalAddresses()
{
    QList<QHostAddress> addresses;
        addresses << QHostAddress::LocalHost << QHostAddress::LocalHostIPv6;
        for (const QNetworkInterface &iface : QNetworkInterface::allInterfaces())
        {
            for (const QNetworkAddressEntry &entry : iface.addressEntries())
            {
                QHostAddress addr = entry.ip();
                if (!addr.isNull() && !addresses.contains(addr))
                {
                    addresses << addr;
                }
            }
        }
        return addresses;
}
