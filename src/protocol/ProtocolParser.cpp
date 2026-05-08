#include "ProtocolParser.h"
#include "../utils/ProtocolConstants.h"
#include "../utils/Utils.h"
#include "../utils/Logger.h"
#include <QtEndian>
#include <QHostAddress>
ProtocolParser::ProtocolParser(QObject *parent)
    : QObject(parent)
{
}

bool ProtocolParser::parseFrame(const QByteArray &frame) {
    Logger::instance()->log(Logger::Debug, "ProtocolParser", QString("[Parser in] func: 0x%1 sub: 0x%2").arg(QString::number(static_cast<quint8>(frame[2]), 16, '0')).arg(QString::number(static_cast<quint8>(frame[3]), 16, '0')));
    if (!validateFrame(frame)) {
        emit parseError("帧验证失败");
        return false;
    }

    // 帧验证通过,可以进一步解析
    return true;
}

bool ProtocolParser::validateFrame(const QByteArray &frame) {
    // 检查最小长度
    if (frame.size() < ProtocolConstants::MIN_FRAME_LENGTH) {
        Logger::instance()->log(Logger::Warning, "ProtocolParser",
                                QString("帧长度不足: %1 < %2").arg(frame.size()).arg(ProtocolConstants::MIN_FRAME_LENGTH));
        return false;
    }

    // 检查帧头
    if (!checkFrameHeader(frame)) {
        Logger::instance()->log(Logger::Warning, "ProtocolParser", "帧头验证失败");
        return false;
    }

    // 检查帧尾
    if (!checkFrameTail(frame)) {
        Logger::instance()->log(Logger::Warning, "ProtocolParser", "帧尾验证失败");
        return false;
    }

    return true;
}

bool ProtocolParser::parsePortConnInfoResponse(const QByteArray &p, QList<PortConnInfo> &list)
{
    list.clear();
    if (p.size() < 2) {
        Logger::instance()->log(Logger::Warning, "ProtocolParser", "端口连接信息响应长度不足");
        return false;
    }
    int n = qFromBigEndian<quint16>(p.constData());
    Logger::instance()->log(Logger::Debug, "ProtocolParser", QString("parsePortConnInfoResponse: 端口数量=%1").arg(n));
    int off = 2;
    for (int i = 0; i < n && off + 21 <= p.size(); ++i) {
        PortConnInfo info;
        info.portIndex = p[off];
        info.locked    = p[off + 2] == 0;
        info.modeText  = QStringList{"RealCOM", "TCP Server", "Redundant"}.value(p[off + 1], "?");
        /* 后面 16 字节是 4 个 IP，按需解析 */
        info.connIPs.clear();
        for (int k = 0; k < 4; ++k) {
            quint32 ip = qFromBigEndian<quint32>(p.constData() + off + 3 + k * 4);
            if (ip) info.connIPs << QHostAddress(ip).toString();
        }
        list.append(info);
        off += 21;
    }
    Logger::instance()->log(Logger::Debug, "ProtocolParser", QString("成功解析端口连接信息: %1个端口").arg(list.size()));
    return true;
}

bool ProtocolParser::parsePortStatusResponse(const QByteArray &p, QList<PortStatusInfo> &list)
{
    list.clear();
    if (p.size() < 2) {
        Logger::instance()->log(Logger::Warning, "ProtocolParser", "端口状态响应长度不足");
        return false;
    }
    int n = qFromBigEndian<quint16>(p.constData());
    Logger::instance()->log(Logger::Debug, "ProtocolParser", QString("parsePortStatusResponse: 端口数量=%1").arg(n));
    int off = 2;
    for (int i = 0; i < n && off + 25 <= p.size(); ++i) {
        PortStatusInfo info;
        info.portIndex     = p[off];
        info.txCount       = qFromBigEndian<quint32>(p.constData() + off + 1);
        info.rxCount       = qFromBigEndian<quint32>(p.constData() + off + 5);
        info.txTotalCount  = qFromBigEndian<quint64>(p.constData() + off + 9);
        info.rxTotalCount  = qFromBigEndian<quint64>(p.constData() + off + 17);
        list.append(info);
        off += 25;
    }
    Logger::instance()->log(Logger::Debug, "ProtocolParser", QString("成功解析端口状态: %1个端口").arg(list.size()));
    return true;
}

bool ProtocolParser::parsePortErrorResponse(const QByteArray &p, QList<PortErrorInfo> &list)
{
    list.clear();
    if (p.size() < 2) {
        Logger::instance()->log(Logger::Warning, "ProtocolParser", "端口错误响应长度不足");
        return false;
    }
    int n = qFromBigEndian<quint16>(p.constData());
    Logger::instance()->log(Logger::Debug, "ProtocolParser", QString("parsePortErrorResponse: 端口数量=%1").arg(n));
    int off = 2;
    for (int i = 0; i < n && off + 17 <= p.size(); ++i) {
        PortErrorInfo info;
        info.portIndex  = p[off];
        info.frameErr   = qFromBigEndian<quint32>(p.constData() + off + 1);
        info.parityErr  = qFromBigEndian<quint32>(p.constData() + off + 5);
        info.overrunErr = qFromBigEndian<quint32>(p.constData() + off + 9);
        info.breakErr   = qFromBigEndian<quint32>(p.constData() + off + 13);
        list.append(info);
        off += 17;
    }
    Logger::instance()->log(Logger::Debug, "ProtocolParser", QString("成功解析端口错误: %1个端口").arg(list.size()));
    return true;
}

bool ProtocolParser::checkFrameHeader(const QByteArray &frame) {
    if (frame.size() < 2) {
        Logger::instance()->log(Logger::Warning, "ProtocolParser", "帧头检查：帧长度不足");
        return false;
    }

    quint16 header = qFromBigEndian<quint16>(reinterpret_cast<const uchar*>(frame.constData()));
    if (header != ProtocolConstants::FRAME_HEADER) {
        Logger::instance()->log(Logger::Warning, "ProtocolParser",
            QString("帧头检查：帧头错误: 0x%1 != 0x%2")
                .arg(header, 4, 16, QChar('0'))
                .arg(ProtocolConstants::FRAME_HEADER, 4, 16, QChar('0')));
        return false;
    }
    return true;
}

bool ProtocolParser::checkFrameTail(const QByteArray &frame) {
    if (frame.size() < 2) {
        Logger::instance()->log(Logger::Warning, "ProtocolParser", "帧尾检查：帧长度不足");
        return false;
    }

    quint16 tail = qFromBigEndian<quint16>(
                reinterpret_cast<const uchar*>(frame.constData() + frame.size() - 2));
    if (tail != ProtocolConstants::FRAME_TAIL) {
        Logger::instance()->log(Logger::Warning, "ProtocolParser",
            QString("帧尾检查：帧尾错误: 0x%1 != 0x%2")
                .arg(tail, 4, 16, QChar('0'))
                .arg(ProtocolConstants::FRAME_TAIL, 4, 16, QChar('0')));
        return false;
    }
    return true;
}

bool ProtocolParser::parseOverviewResponse(const QByteArray &payload, DeviceInfo &info)
{
    if (payload.size() < 141)
    {          // 你的最小长度
        Logger::instance()->log(Logger::Error, "ProtocolParser", "Overview payload too short: " + QString::number(payload.size()));
        emit parseError("Overview payload too short");
        return false;
    }

    /* 1. 型号 / 名称 / 序列号 ASCII ------------------------------------*/
    info.deviceModel = QString::fromLatin1(payload.mid(0, 40)).trimmed();
    info.deviceName  = QString::fromLatin1(payload.mid(48, 40)).trimmed();

    /* 序列号：8 字节 ASCII → 16 进制字符串 yyyyMMddnnn */
    QByteArray snRaw = payload.mid(40, 8);
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
    info.serialNum = datastr;


    /* 2. 固件版本 4B 小端 → 8 位 HEX -----------------------------------*/
    // 假设 fwRaw 格式：0x01000001 → 主版本1，次版本1
    // 修复类型转换错误，提取固件版本
    quint32 fwRaw = qFromLittleEndian<quint32>(reinterpret_cast<const uchar*>(payload.mid(88, 4).constData()));

    // 将固件版本转换为 V1.001 格式
    quint8 mainVer = (fwRaw >> 24) & 0xFF;  // 主版本（第1字节）
    quint16 subVer = fwRaw & 0x00FFFFFF;    // 次版本（后3字节）
    info.firmwareVer = QString("V%1.%2").arg(mainVer).arg(subVer, 3, 10, QChar('0'));

    /* 3. 单字节枚举 -----------------------------------------------------*/
    info.deviceState   = static_cast<QPORT_DeviceState>(static_cast<quint8>(payload[92]));
    info.validLanCount = static_cast<quint8>(payload[93]);

    /* 4. IP 地址（大端） -----------------------------------------------*/
    auto ipFromLE = [&payload](int off) -> QString
    {
        quint32 ip = qFromBigEndian<quint32>(reinterpret_cast<const uchar*>(payload.constData() + off));
        return QHostAddress(ip).toString();
    };
    info.lan1Ip = ipFromLE(94);
    info.lan2Ip = ipFromLE(105);

    /* 5. MAC 地址（6 字节原序） ----------------------------------------*/
    auto macFrom6 = [&payload](int off) -> QString
    {
        QStringList parts;
        for (int i = 0; i < 6; ++i)
        {
            quint8 b = static_cast<quint8>(payload[off + i]);
            parts << QString("%1").arg(b, 2, 16, QChar('0')).toUpper();
        }
        return parts.join(":");
    };
    info.lan1Mac = macFrom6(98);
    info.lan2Mac = macFrom6(109);

    /* 6. 链路 / 电源 / LCD / 锁定 --------------------------------------*/
    info.lan1LinkState = static_cast<QPORT_LanLinkState>(static_cast<quint8>(payload[104]));
    info.lan2LinkState = static_cast<QPORT_LanLinkState>(static_cast<quint8>(payload[115]));
    info.lcdState      = static_cast<QPORT_LCDState>(static_cast<quint8>(payload[116]));
    info.power1State   = static_cast<QPORT_PowerState>(static_cast<quint8>(payload[117]));
    info.power2State   = static_cast<QPORT_PowerState>(static_cast<quint8>(payload[118]));
    info.devLockState  = static_cast<QPORT_DeviceLockState>(static_cast<quint8>(payload[139]));

    /* 7. 接口类型 / 状态 / 锁定位图 ------------------------------------*/
    info.ifTypeCount = QString::fromLatin1(payload.mid(119, 4)).trimmed();
    for (int i = 0; i < 16; ++i) info.ifStatus[i]     = static_cast<quint8>(payload[123 + i]);
    for (int i = 0; i < 16; ++i) info.ifLockStatus[i] = static_cast<quint8>(payload[140 + i]);

    Logger::instance()->log(Logger::Info, "ProtocolParser",
                            QString("Overview parsed: %1 (%2)").arg(info.deviceName, info.serialNum));

    /* 8. 端口解析（固定 14 字节 × 16 个） -------------------------------*/
    int baseOff   = 156;
    int portLen   = 14;
    info.ports.clear();
    for (int i = 0; i < 16; ++i)
    {
        int off = baseOff + i * portLen;
        PortStatus ps;

        ps.portNum   = static_cast<quint8>(payload[off]);
        ps.baudRate  = qFromLittleEndian<quint32>(reinterpret_cast<const uchar*>(payload.constData() + off + 1));
        ps.dataBits  = static_cast<quint8>(payload[off + 5]);
        ps.stopBits  = static_cast<quint8>(payload[off + 6]);
        ps.parity    = static_cast<PortParity>(static_cast<quint8>(payload[off + 7]));
        ps.workStatus= static_cast<PortWorkStatus>(static_cast<quint8>(payload[off + 8]));
        ps.lockStatus= static_cast<PortLockStatus>(static_cast<quint8>(payload[off + 9]));
        ps.ifType    = static_cast<SerialPortInterfaceType>(static_cast<quint8>(payload[off + 10]));
        ps.workMode  = static_cast<PortWorkMode>(static_cast<quint8>(payload[off + 11]));

        info.ports.append(ps);
    }

    emit overviewParsed(info);
    return true;
}

bool ProtocolParser::parseNetworkConfigResponse(const QByteArray &payload, NetworkConfig &config)
{
    // 验证payload长度 (至少需要基本网络配置字段)
    Logger::instance()->log(Logger::Debug, "ProtocolParser", QString("[Net] payload size %1 data: %2").arg(payload.size()).arg(QString(payload.toHex(' ').toUpper())));
    if (payload.size() < 30)
    {
        Logger::instance()->log(Logger::Error, "ProtocolParser",
                                QString("网络配置响应长度不足: %1 < 50").arg(payload.size()));
        emit parseError("网络配置响应长度不足");
        return false;
    }

    int offset = 0;

    // 解析网口数量
    config.lanCount = Utils::safeRead<quint8>(payload, offset, 0);
    offset += 1;

    // 解析LAN1配置
    config.lan1UseDhcp = Utils::safeRead<quint8>(payload, offset, 0) != 0;
    offset += 1;

    config.lan1Ip = Utils::parseIpAddress(payload, offset);
    offset += 4;

    config.lan1Netmask = Utils::parseIpAddress(payload, offset);
    offset += 4;

    config.lan1Gateway = Utils::parseIpAddress(payload, offset);
    offset += 10;

    config.lan1Speed = Utils::safeRead<quint8>(payload, offset, 0);
    offset += 1;

    // 解析LAN2配置
    if (offset + 14 <= payload.size())
    {
        config.lan2UseDhcp = Utils::safeRead<quint8>(payload, offset, 0) != 0;
        offset += 1;

        config.lan2Ip = Utils::parseIpAddress(payload, offset);
        offset += 4;

        config.lan2Netmask = Utils::parseIpAddress(payload, offset);
        offset += 4;

        config.lan2Gateway = Utils::parseIpAddress(payload, offset);
        offset += 10;

        config.lan2Speed = Utils::safeRead<quint8>(payload, offset, 0);
        offset += 7;
    }


    Logger::instance()->log(Logger::Info, "ProtocolParser",
                            QString("成功解析网络配置: LAN1=%1, LAN2=%2").arg(config.lan1Ip).arg(config.lan2Ip));

    return true;
}

bool ProtocolParser::parseSerialConfigResponse(const QByteArray &payload, QList<SerialPortConfig> &configs) {
    using namespace SerialConfigOffset;
    configs.clear();
    // 验证payload长度
    if (payload.size() != BLOCK_SIZE)
    {
        Logger::instance()->log(Logger::Error, "ProtocolParser",
                                QString("串口配置响应长度不足: %1 != %2").arg(payload.size()).arg(BLOCK_SIZE));
        emit parseError("串口配置响应长度不足");
        return false;
    }
    // 计算配置块数量
    int blockCount = payload.size() / BLOCK_SIZE;
    for (int i = 0; i < blockCount; ++i)
    {
        int offset = i * BLOCK_SIZE;
        SerialPortConfig config;
        // 解析端口索引
        config.index = static_cast<int>(payload[offset + PORT_INDEX]);
        // 解析别名长度和别名
        quint8 aliasLen = static_cast<quint8>(payload[offset + ALIAS_LEN]);
        if (aliasLen > 0 && aliasLen <= ALIAS_MAX_LEN)
        {
            config.alias = QString::fromUtf8(payload.mid(offset + ALIAS_LEN + 1, aliasLen)).trimmed();
        }
        // 解析波特率 (4字节,小端序)
        config.baudRate = qFromLittleEndian<quint32>(reinterpret_cast<const uchar*>(payload.constData() + offset + BAUD_RATE));
        // 解析数据位
        config.dataBits = static_cast<quint8>(payload[offset + DATA_BITS]);
        // 解析停止位
        config.stopBits = static_cast<quint8>(payload[offset + STOP_BITS]);
        // 解析校验位
        config.parity = static_cast<quint8>(payload[offset + PARITY]);
        // 解析流控
        config.flowControl = static_cast<quint8>(payload[offset + FLOW_CTRL]);
        // 解析接口类型
//        config.interface = static_cast<quint8>(payload[offset + INTERFACE]);
        // 移动到下一个配置块
        offset = offset + BLOCK_SIZE;
        configs.append(config);
    }

    Logger::instance()->log(Logger::Info, "ProtocolParser",
                            QString("成功解析串口配置: %1个端口").arg(configs.size()));
    return true;
}

bool ProtocolParser::parsePortLockStatusResponse(const QByteArray &payload,
                                                 QList<PortLockInfo> &lockStatus) {
    Logger::instance()->log(Logger::Debug, "ProtocolParser", QString("[Lock] payload size %1").arg(payload.size()));

    lockStatus.clear();
    const int BLOCK = 7;                 // 协议：1 端口号 + 1 锁定 + 5 MAC
    int cnt = payload.size() / BLOCK;    // 向下取整，防止尾垫 0

    for (int i = 0; i < cnt; ++i) {
        int off = i * BLOCK;

        PortLockInfo info;
        info.portNum  = static_cast<quint8>(payload[off]);            // 第 0 字节
        info.lockState = static_cast<quint8>(payload[off + 1]);       // 第 1 字节
        info.lockMac   = QString::fromLatin1(payload.mid(off + 2, 5)) // 第 2~6 字节（5 字节 ASCII）
                .trimmed();

        lockStatus[info.portNum] = info;   // 按端口号索引
    }

    Logger::instance()->log(Logger::Debug, "ProtocolParser", QString("[Lock] parsed %1 ports").arg(lockStatus.size()));
    emit portLockStatusParsed(lockStatus);   // 转 QList 发射
    return true;
}

bool ProtocolParser::parseStatusMonitorResponse(const QByteArray &payload, QList<PortStatusInfo> &status) {
    using namespace StatusMonitorOffset;

    status.clear();

    // 验证payload长度
    if (payload.size() < BLOCK_SIZE) {
        Logger::instance()->log(Logger::Error, "ProtocolParser",
                                QString("状态监控响应长度不足: %1 < %2").arg(payload.size()).arg(BLOCK_SIZE));
        emit parseError("状态监控响应长度不足");
        return false;
    }

    // 计算状态块数量
    int blockCount = payload.size() / BLOCK_SIZE;

    for (int i = 0; i < blockCount; ++i) {
        int offset = i * BLOCK_SIZE;

        PortStatusInfo info;

        // 解析端口索引
        info.portIndex = Utils::safeRead<quint8>(payload, offset + PORT_INDEX, 0);

        // 解析发送计数 (4字节)
        info.txCount = Utils::fromBigEndian<quint32>(payload, offset + TX_CNT);

        // 解析接收计数 (4字节)
        info.rxCount = Utils::fromBigEndian<quint32>(payload, offset + RX_CNT);

        // 解析总发送计数 (8字节)
        info.txTotalCount = Utils::fromBigEndian<quint64>(payload, offset + TX_TOTAL_CNT);

        // 解析总接收计数 (8字节)
        info.rxTotalCount = Utils::fromBigEndian<quint64>(payload, offset + RX_TOTAL_CNT);

        status.append(info);
    }

    Logger::instance()->log(Logger::Info, "ProtocolParser",
                            QString("成功解析状态监控: %1个端口").arg(status.size()));

    emit statusMonitorParsed(status);
    return true;
}
