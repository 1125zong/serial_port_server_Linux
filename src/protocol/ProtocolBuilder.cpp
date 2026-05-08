#include "ProtocolBuilder.h"
#include "../utils/ProtocolConstants.h"
#include "../utils/Utils.h"
#include "../utils/Logger.h"
#include <QtEndian>

QByteArray ProtocolBuilder::buildFrame(quint8 funcId, quint8 subFuncId, const QByteArray &payload) {
    QByteArray frame;
    Logger::instance()->log(Logger::Debug, "ProtocolBuilder", QString("BuildFrame payload: %1").arg(QString(payload.toHex(' '))));
    // 添加帧头 (大端序)
    quint16 header = ProtocolConstants::FRAME_HEADER;
    QByteArray headerBytes(2, 0);
    qToBigEndian(header, reinterpret_cast<uchar*>(headerBytes.data()));
    frame.append(headerBytes);
    
    // 添加功能ID
    frame.append(static_cast<char>(funcId));
    
    // 添加子功能ID
    frame.append(static_cast<char>(subFuncId));
    
    // 添加payload
    frame.append(payload);
    
    // 添加帧尾 (大端序)
    quint16 tail = ProtocolConstants::FRAME_TAIL;
    QByteArray tailBytes(2, 0);
    qToBigEndian(tail, reinterpret_cast<uchar*>(tailBytes.data()));
    frame.append(tailBytes);
    Logger::instance()->log(Logger::Debug, "ProtocolBuilder", QString("BuildFrame Frame: %1").arg(QString(frame.toHex(' '))));
    return frame;
}

QByteArray ProtocolBuilder::ipStringToBytes(const QString &ip)
{
    Logger::instance()->log(Logger::Debug, "ProtocolBuilder", QString("QByteArray ProtocolBuilder::ipStringToBytes(const QString &ip) %1").arg(ip));
    return Utils::ipStringToBytes(ip);
}

QByteArray ProtocolBuilder::macStringToBytes(const QString &mac) {
    Logger::instance()->log(Logger::Debug, "ProtocolBuilder", QString("macStringToBytes: %1").arg(mac));
    return Utils::macStringToBytes(mac);
}

// 构建概述查询命令
QByteArray ProtocolBuilder::buildOverviewQuery() {
    // 功能ID: 0x01 (概述), 子功能ID: 0x01 (读取概述信息)
    return buildFrame(0x01, 0x01, QByteArray());
}

// 构建网络配置查询命令
QByteArray ProtocolBuilder::buildNetworkConfigQuery()
{
    // 功能ID: 0x03 (网络设置), 子功能ID: 0x01 (读取网络配置)
    return buildFrame(0x03, 0x01, QByteArray());
}

// 构建网络配置写入命令
QByteArray ProtocolBuilder::buildNetworkConfigWrite(const NetworkConfig &config) {
    QByteArray payload;
    
    // 网口数量
    payload.append(static_cast<char>(config.lanCount));
    
    // LAN1配置
    payload.append(static_cast<char>(config.lan1UseDhcp ? 2 : 1));
    payload.append(ipStringToBytes(config.lan1Ip));
    payload.append(ipStringToBytes(config.lan1Netmask));
    payload.append(ipStringToBytes(config.lan1Gateway));
    
    // LAN2配置
    payload.append(static_cast<char>(config.lan2UseDhcp ? 2 : 1));
    payload.append(ipStringToBytes(config.lan2Ip));
    payload.append(ipStringToBytes(config.lan2Netmask));
    payload.append(ipStringToBytes(config.lan2Gateway));
    
    Logger::instance()->log(Logger::Debug, "ProtocolBuilder", QString("buildNetworkConfigWrite: LAN count=%1, LAN1 DHCP=%2, LAN1 IP=%3, LAN2 DHCP=%4, LAN2 IP=%5")
        .arg(config.lanCount)
        .arg(config.lan1UseDhcp)
        .arg(config.lan1Ip)
        .arg(config.lan2UseDhcp)
        .arg(config.lan2Ip));
    
    // 功能ID: 0x03 (网络设置), 子功能ID: 0x02 (写入网络配置)
    return buildFrame(0x03, 0x02, payload);
}

// 构建串口配置查询命令
QByteArray ProtocolBuilder::buildSerialConfigQuery(int portIndex)
{
    QByteArray payload;
    QByteArray indexBytes;
    if (portIndex == 1616)
    {
        indexBytes = QByteArray::fromHex("FF");
    }
    else
    {
        indexBytes = QByteArray(1, static_cast<char>(portIndex));
    }
    payload.append(indexBytes);
    
    // 功能ID: 0x04 (串口设置), 子功能ID: 0x01 (读取串口配置)
    return buildFrame(0x04, 0x01, payload);
}

// 构建单个串口配置写入命令
QByteArray ProtocolBuilder::buildSerialConfigWrite(const SerialPortConfig &config) {
    QByteArray payload;
    
    // 端口索引
    payload.append(static_cast<char>(config.index));

    // 别名的长度
    QByteArray aliasLen = QByteArray::fromHex((QString("%1").arg(config.alias.length(), 2, 16, QChar('0'))).toUtf8());
    payload.append(aliasLen);

    // 别名
    for(int i = 0; i < config.alias.length(); i++)
    {
        QChar ch = config.alias.at(i);
        QString hex = QString("%1").arg(ch.unicode(), 2, 16, QChar('0'));
        QByteArray font = QByteArray::fromHex(hex.toUtf8());
        payload.append(font);
    }

    // 波特率 (4字节,大端序)
    QByteArray baudBytes(4, 0);
    qToBigEndian(config.baudRate, reinterpret_cast<uchar*>(baudBytes.data()));
    payload.append(baudBytes);
    
    // 数据位
    payload.append(static_cast<char>(config.dataBits));
    
    // 停止位
    payload.append(static_cast<char>(config.stopBits));
    
    // 校验位
    payload.append(static_cast<char>(config.parity));
    
    // 流控
    payload.append(static_cast<char>(config.flowControl));
    
    // 接口类型
    payload.append(static_cast<char>(config.interface));
    
    Logger::instance()->log(Logger::Debug, "ProtocolBuilder", QString("buildSerialConfigWrite: Port=%1, Alias=%2, Baud=%3, DataBits=%4, StopBits=%5, Parity=%6, FlowControl=%7, Interface=%8")
        .arg(config.index)
        .arg(config.alias)
        .arg(config.baudRate)
        .arg(config.dataBits)
        .arg(config.stopBits)
        .arg(config.parity)
        .arg(config.flowControl)
        .arg(config.interface));
    
    // 功能ID: 0x04 (串口设置), 子功能ID: 0x02 (写入单个串口)
    return buildFrame(0x04, 0x02, payload);
}

QByteArray ProtocolBuilder::buildSerialConfigWrite1(const SerialPortConfig &config)
{
    QByteArray payload;

    payload.append(config.syncBitmap);  //包含了同步的端口个数及其端口号

    // 波特率 (4字节,大端序)
    QByteArray baudBytes(4, 0);
    qToBigEndian(config.baudRate, reinterpret_cast<uchar*>(baudBytes.data()));
    payload.append(baudBytes);

    // 数据位
    payload.append(static_cast<char>(config.dataBits));

    // 停止位
    payload.append(static_cast<char>(config.stopBits));

    // 校验位
    payload.append(static_cast<char>(config.parity));

    // 流控
    payload.append(static_cast<char>(config.flowControl));

    // 接口类型
    payload.append(static_cast<char>(config.interface));

    Logger::instance()->log(Logger::Debug, "ProtocolBuilder", QString("buildSerialConfigWrite1: SyncBitmap=%1, Baud=%2, DataBits=%3, StopBits=%4, Parity=%5, FlowControl=%6, Interface=%7")
        .arg(QString(config.syncBitmap.toHex(' ')))
        .arg(config.baudRate)
        .arg(config.dataBits)
        .arg(config.stopBits)
        .arg(config.parity)
        .arg(config.flowControl)
        .arg(config.interface));

    // 功能ID: 0x04 (串口设置), 子功能ID: 0x03 (写入多个串口)
    return buildFrame(0x04, 0x03, payload);
}

// 构建多串口配置写入命令
QByteArray ProtocolBuilder::buildMultiSerialConfigWrite(const QList<SerialPortConfig> &configs) {
    QByteArray payload;
    
    // 串口数量
    payload.append(static_cast<char>(configs.size()));
    
    // 添加每个串口配置
    for (const SerialPortConfig &config : configs) {
        // 端口索引
        payload.append(static_cast<char>(config.index));
        
        // 别名长度和别名
        QByteArray aliasBytes = config.alias.toUtf8();
        quint8 aliasLen = qMin(aliasBytes.length(), 19);
        payload.append(static_cast<char>(aliasLen));
        payload.append(aliasBytes.left(aliasLen));
        payload.append(QByteArray(19 - aliasLen, 0));
        
        // 波特率
        QByteArray baudBytes(4, 0);
        qToBigEndian(config.baudRate, reinterpret_cast<uchar*>(baudBytes.data()));
        payload.append(baudBytes);
        
        // 其他参数
        payload.append(static_cast<char>(config.dataBits));
        payload.append(static_cast<char>(config.stopBits));
        payload.append(static_cast<char>(config.parity));
        payload.append(static_cast<char>(config.flowControl));
        payload.append(static_cast<char>(config.fifo));
        payload.append(static_cast<char>(config.interface));
    }
    
    Logger::instance()->log(Logger::Debug, "ProtocolBuilder", QString("buildMultiSerialConfigWrite: Port count=%1").arg(configs.size()));
    
    // 功能ID: 0x04 (串口设置), 子功能ID: 0x03 (写入多个串口)
    return buildFrame(0x04, 0x03, payload);
}

// 构建模式配置写入命令
QByteArray ProtocolBuilder::buildModeConfigWrite(int portIndex, quint8 mode, const QVariantMap &params) {
    QByteArray payload;
    
    // 端口索引
    payload.append(static_cast<char>(portIndex));
    
    // 工作模式
    payload.append(static_cast<char>(mode));
    
    int conNum = 0;
    int Time = 0;
    QByteArray TcpConNumAndTime;
    QString NT;
    switch (mode)
    {
    case 0x01:
        Time = params["RealComModeTCPaliveCheckTime"].toUInt();
        conNum = params["RealComModeTCPmaxConnectNum"].toUInt();
        Logger::instance()->log(Logger::Debug, "ProtocolBuilder", QString("Time: %1 conNum：%2").arg(Time).arg(conNum));
        TcpConNumAndTime = QByteArray::fromHex((QString("%1%2").arg(conNum, 2, 16, QChar('0')).arg(Time, 2, 16, QChar('0'))).toUtf8());   // 结果为字符串"0x0x  numTime"
        Logger::instance()->log(Logger::Debug, "ProtocolBuilder", QString("TcpConNumAndTime：%1").arg(QString(TcpConNumAndTime.toHex(' '))));
        break;
    case 0x03:
        Time = params["TCPServerModeTCPaliveCheckTime"].toUInt();
        conNum = params["TCPServerTCPmaxConnectNum"].toUInt();
        TcpConNumAndTime = QByteArray::fromHex((QString("%1%2").arg(conNum, 2, 16, QChar('0')).arg(Time, 2, 16, QChar('0'))).toUtf8());   // 结果为字符串"0x0x"
        break;
        //    case 0x05:
        //        param[len++] = params["group"].toUInt();
        //        break;
    }
    payload.append(TcpConNumAndTime);

    // 功能ID: 0x05 (串口模式设置), 子功能ID: 0x01 (写入模式配置)
    return buildFrame(0x05, mode, payload);
}

QByteArray ProtocolBuilder::buildModeConfigWrite1(quint8 mode, const QVariantMap &params,const SerialPortConfig &config)
{
    QByteArray payload;

    payload.append(config.syncBitmap);
    payload.append(static_cast<char>(mode));

    //    payload.append(static_cast<char>(portIndex));


    /* 1. 模式参数（固定 8 字节，未用补 0） */
    int conNum = 0;
    int Time = 0;
    QByteArray TcpConNumAndTime;

    switch (mode)
    {
    case 0x02:
        Time = params["RealComModeTCPaliveCheckTime"].toUInt();
        conNum = params["RealComModeTCPmaxConnectNum"].toUInt();
        TcpConNumAndTime = QByteArray::fromHex((QString("%1%2").arg(conNum, 2, 16, QChar('0')).arg(Time, 2, 16, QChar('0'))).toUtf8());   // 结果为字符串"0x0x  numTime"
        break;
    case 0x04:
        Time = params["TCPServerModeTCPaliveCheckTime"].toUInt();
        conNum = params["TCPServerTCPmaxConnectNum"].toUInt();
        TcpConNumAndTime = QByteArray::fromHex((QString("%1%2").arg(conNum, 2, 16, QChar('0')).arg(Time, 2, 16, QChar('0'))).toUtf8());   // 结果为字符串"0x0x"
        break;
        //    case 0x05:
        //        param[len++] = params["group"].toUInt();
        //        break;
    }
    payload.append(TcpConNumAndTime);

    Logger::instance()->log(Logger::Debug, "ProtocolBuilder", QString("buildModeConfigWrite1: Mode=%1, SyncBitmap=%2, ConNum=%3, Time=%4")
        .arg(mode)
        .arg(QString(config.syncBitmap.toHex(' ')))
        .arg(conNum)
        .arg(Time));

    /* 2. 同步位图（固定 32 字节） */

    return buildFrame(0x05, mode, payload);
}

QByteArray ProtocolBuilder::buildPortQuery(const int portIndex)
{
    QByteArray payload = QByteArray::fromHex(QString("%1").arg((portIndex), 2, 16, QChar('0')).toUtf8());
    Logger::instance()->log(Logger::Debug, "ProtocolBuilder", QString("buildPortQuery: Port index=%1").arg(portIndex));
    return buildFrame(0x06, 0x01, payload);
}

QByteArray ProtocolBuilder::buildPortSpecifiedMode(const QByteArray &mode)
{
    QByteArray payload;
    payload.append(mode);
    Logger::instance()->log(Logger::Debug, "ProtocolBuilder", QString("buildPortSpecifiedMode: Mode=%1").arg(QString(mode.toHex(' '))));
    return buildFrame(0x06, 0x02, payload);
}

// 构建端口重置命令
QByteArray ProtocolBuilder::buildPortReset(quint16 portMask) {
    QByteArray payload;
    
    // 端口掩码 (2字节,大端序)
    QByteArray maskBytes(2, 0);
    qToBigEndian(portMask, reinterpret_cast<uchar*>(maskBytes.data()));
    payload.append(maskBytes);
    
    Logger::instance()->log(Logger::Debug, "ProtocolBuilder", QString("buildPortReset: Port mask=0x%1").arg(portMask, 4, 16, QChar('0')));
    
    // 功能ID: 0x08 (设备管理), 子功能ID: 0x01 (重置端口)
    return buildFrame(0x08, 0x01, payload);
}
QByteArray ProtocolBuilder::buildSelfCheckWithSync1(const QByteArray &syncBitmap)
{
    // 功能 ID = 0x08 设备管理，子功能 = 0x01 重置
    // 负载 = 端口个数 + 端口号
    QByteArray payload;
    payload.append(syncBitmap);
    Logger::instance()->log(Logger::Debug, "ProtocolBuilder", QString("buildSelfCheckWithSync1: SyncBitmap=%1").arg(QString(syncBitmap.toHex(' '))));
    return buildFrame(0x08, 0x01, payload);
}


// 构建端口锁定命令
QByteArray ProtocolBuilder::buildPortLock(int portIndex) {
    QByteArray payload;
    
    // 端口索引
    payload.append(static_cast<char>(portIndex));
    
    Logger::instance()->log(Logger::Debug, "ProtocolBuilder", QString("buildPortLock: Port index=%1").arg(portIndex));
    
    // 功能ID: 0x08 (设备管理), 子功能ID: 0x02 (锁定端口)
    return buildFrame(0x08, 0x02, payload);
}

// 构建端口解锁命令
QByteArray ProtocolBuilder::buildPortUnlock(int portIndex) {
    QByteArray payload;
    
    // 端口索引
    payload.append(static_cast<char>(portIndex));
    
    Logger::instance()->log(Logger::Debug, "ProtocolBuilder", QString("buildPortUnlock: Port index=%1").arg(portIndex));
    
    // 功能ID: 0x08 (设备管理), 子功能ID: 0x03 (解锁端口)
    return buildFrame(0x08, 0x03, payload);
}

// 构建设备锁定命令
QByteArray ProtocolBuilder::buildDeviceLock()
{
    // 功能ID: 0x08 (设备管理), 子功能ID: 0x06 (锁定设备)
    QByteArray payload = getMacAddress();
    Logger::instance()->log(Logger::Debug, "ProtocolBuilder", QString("ProtocolBuilder::buildDeviceLock payload %1").arg(QString(payload.toHex(' '))));
    return buildFrame(0x08, 0x06, payload);
}

// 构建设备解锁命令
QByteArray ProtocolBuilder::buildDeviceUnlock(const QString &code)
{
    QByteArray payload;
    // 解锁码
    payload.append(code.toUtf8()).append(getMacAddress());
    
    Logger::instance()->log(Logger::Debug, "ProtocolBuilder", QString("buildDeviceUnlock: Code length=%1").arg(code.length()));
    
    // 功能ID: 0x08 (设备管理), 子功能ID: 0x07 (解锁设备)
    return buildFrame(0x08, 0x07, payload);
}

// 构建恢复出厂设置命令
QByteArray ProtocolBuilder::buildFactoryReset()
{
    Logger::instance()->log(Logger::Debug, "ProtocolBuilder", "buildFactoryReset: Restoring factory settings");
    // 功能ID: 0x08 (设备管理), 子功能ID: 0x0A (恢复出厂设置)
    return buildFrame(0x08, 0x0A, QByteArray());
}

// 构建重启命令
QByteArray ProtocolBuilder::buildReboot() {
    Logger::instance()->log(Logger::Debug, "ProtocolBuilder", "buildReboot: Rebooting device");
    // 功能ID: 0x08 (设备管理), 子功能ID: 0x0C (保存并重启)
    // 注意: 协议中恢复出厂和重启使用相同的子功能ID,需要通过payload区分
    //    QByteArray payload;
    //    payload.append(static_cast<char>(0x01));  // 标识为重启操作
    
    return buildFrame(0x08, 0x0C, QByteArray());
}

QByteArray ProtocolBuilder::buildSelfCheck(quint8 portByte)
{
    Logger::instance()->log(Logger::Debug, "ProtocolBuilder", QString("buildSelfCheck: Port byte=0x%1").arg(portByte, 2, 16, QChar('0')));
    return buildFrame(0x08, 0x08, QByteArray(1, portByte));
}
QByteArray ProtocolBuilder::buildSelfCheckWithSync(const QByteArray &syncBitmap)
{
    // 功能 ID = 0x08 设备管理，子功能 = 0x08 自检
    // 负载 = 端口个数 + 端口号
    QByteArray payload;
    payload.append(syncBitmap);                // 实际字节，最大为 16B
    Logger::instance()->log(Logger::Debug, "ProtocolBuilder", QString("buildSelfCheckWithSync: SyncBitmap=%1").arg(QString(syncBitmap.toHex(' '))));
    return buildFrame(0x08, 0x08, payload);
}

QByteArray ProtocolBuilder::buildDeviceManageCommand(quint8 subFunc, const QByteArray &pld)
{
    Logger::instance()->log(Logger::Debug, "ProtocolBuilder", QString("buildDeviceManageCommand: SubFunc=0x%1, Payload length=%2").arg(subFunc, 2, 16, QChar('0')).arg(pld.size()));
    return buildFrame(0x08, subFunc, pld);   // 内部继续调私有 buildFrame
}

QByteArray ProtocolBuilder::buildStatusCommand(quint8 subFunc, const QByteArray &pld)
{
    Logger::instance()->log(Logger::Debug, "ProtocolBuilder", QString("buildStatusCommand: SubFunc=0x%1, Payload length=%2").arg(subFunc, 2, 16, QChar('0')).arg(pld.size()));
    return buildFrame(0x07, subFunc, pld);   // 调私有实现
}





QByteArray ProtocolBuilder::buildMultiPortLock(const QByteArray &bitmap, bool lock)
{
    // 功能 0x08 子功能 0x02/0x03，负载 = 1 字节命令 + 32 字节位图
    QByteArray payload;
    QByteArray mac = getMacAddress();

    payload.append(bitmap); // 固定 32 字节
    payload.append(mac);
    Logger::instance()->log(Logger::Debug, "ProtocolBuilder", QString("buildMultiPortLock: Lock=%1, Bitmap=%2").arg(lock).arg(QString(bitmap.toHex(' '))));
    return buildFrame(0x08, lock ? 0x02 : 0x03, payload);  // 总长度 35 字节
}

QByteArray ProtocolBuilder::buildFinalCheck()
{
    Logger::instance()->log(Logger::Debug, "ProtocolBuilder", "buildFinalCheck: Performing final check");
    // 功能 0x08 子功能 0x05，无负载
    return buildFrame(0x08, 0x05, QByteArray());
}

// 构建端口锁定状态查询命令
QByteArray ProtocolBuilder::buildPortLockStatusQuery() {
    Logger::instance()->log(Logger::Info, "ProtocolBuilder", "buildPortLockStatusQuery: Querying port lock status");
    // 功能ID: 0x08 (设备管理), 子功能ID: 0x04 (查询端口锁定状态)
    return buildFrame(0x08, 0x04, QByteArray());
}

// 构建设备名称写入命令
QByteArray ProtocolBuilder::buildDeviceNameWrite(const QString &name)
{
    QByteArray payload;

    for(int i = 0; i < name.length(); ++i)
    {
        QChar ch = name.at(i);
        // 将每个字符转换为两位十六进制
        QString hex = QString("%1").arg(ch.unicode(), 2, 16, QChar('0'));
        QByteArray font = QByteArray::fromHex(hex.toUtf8());
        payload.append(font);
    }
    QByteArray NameSize = QByteArray::fromHex((QString("%1").arg((payload.size()), 2, 16, QChar('0'))).toUtf8());
    if(payload.size() > 40)
    {
        Logger::instance()->log(Logger::Error, "ProtocolBuilder", "buildDeviceNameWrite lenth > 40");
        return QByteArray();
    }

    // 获取 名字个数+名字
    NameSize.append(payload);
    Logger::instance()->log(Logger::Debug, "ProtocolBuilder", QString("buildDeviceNameWrite: Name=%1, Length=%2").arg(name).arg(payload.size()));

    // 功能ID: 0x01 (概述), 子功能ID: 0x02 (写入设备名称)
    return buildFrame(0x02, 0x01, NameSize);
}

QByteArray ProtocolBuilder::getMacAddress()
{
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    QString mac;
    QByteArray macAddress;

    for (const QNetworkInterface &interface : interfaces)
    {
        // 获取接口标志和属性
        QNetworkInterface::InterfaceFlags flags = interface.flags();

        // 更严格的筛选条件：
        // 1. 接口处于活动状态 (IsUp)
        // 2. 接口正在运行 (IsRunning)
        // 3. 不是回环接口 (IsLoopBack)
        // 4. 不是点对点接口（通常用于VPN或隧道，虽然不是所有VPN都标记为此类型，但这是一个重要筛选）
        // 5. 通常是物理网卡的接口类型：Ethernet, Wifi 等
        if (flags.testFlag(QNetworkInterface::IsUp) &&
                flags.testFlag(QNetworkInterface::IsRunning) &&
                !flags.testFlag(QNetworkInterface::IsLoopBack) &&
                !flags.testFlag(QNetworkInterface::IsPointToPoint)) { // 新增条件：排除点对点接口

            // 进一步检查接口类型，优先选择以太网和无线局域网接口
            QNetworkInterface::InterfaceType type = interface.type();
            if (type != QNetworkInterface::Ethernet && type != QNetworkInterface::Wifi)
            {
                continue; // 如果不是以太网或WiFi接口，则跳过
            }

            // 关键：通过接口名称或描述排除常见的虚拟网卡
            QString interfaceName = interface.humanReadableName().toLower();
            QString hardwareAddr = interface.hardwareAddress();

            // 忽略没有MAC地址的接口
            if (hardwareAddr.isEmpty() || hardwareAddr == "00:00:00:00:00:00")
            {
                continue;
            }

            // 排除名称中包含常见虚拟化关键词的接口[2,3](@ref)
            // 注意：这个列表可以根据你系统上的实际情况扩展
            if (interfaceName.contains("virtual") ||
                    interfaceName.contains("vmware") ||
                    interfaceName.contains("virtualbox") ||
                    interfaceName.contains("hyper-v") ||
                    interfaceName.contains("tunnel") ||
                    interfaceName.contains("loopback") ||
                    interfaceName.contains("bluetooth") || // 排除蓝牙
                    interfaceName.contains("pseudo"))
            {    // 排除一些伪接口
                continue;
            }

            mac = hardwareAddr;
            QString withoutColons = mac.replace(":", "").replace(" ", "").toUpper();
            Logger::instance()->log(Logger::Debug, "ProtocolBuilder", QString("withoutColons: %1 length: %2").arg(withoutColons).arg(withoutColons.length()));
            for(int i = 0; i < withoutColons.length(); i+=2)
            {
                bool ok;
                quint8 byte = withoutColons.mid(i, 2).toUInt(&ok, 16);

                if(!ok)
                {
                    Logger::instance()->log(Logger::Error, "ProtocolBuilder", QString("Invalid byte at index %1: %2").arg(i).arg(withoutColons.mid(i, 2)));
                }
                macAddress.append(byte);
            }
            Logger::instance()->log(Logger::Debug, "ProtocolBuilder", QString("Found potential physical adapter: %1 MAC: %2").arg(interfaceName).arg(QString(macAddress.toHex(' '))));
            // 优先选择以太网接口，因为通常它是有线物理网卡
            if (type == QNetworkInterface::Ethernet)
            {
                break; // 找到以太网卡，认为是最佳选择，退出循环
            }
            // 如果是WiFi，继续循环，看看有没有以太网卡。如果没有，最后找到的WiFi地址也会被返回。
        }
    }
    if (macAddress.isEmpty() || macAddress.size() != 6) {
        Logger::instance()->log(Logger::Error, "ProtocolBuilder", "Invalid MAC address");
        return QByteArray();
    }
    return macAddress;
}


