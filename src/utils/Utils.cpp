#include "Utils.h"
#include <QStringList>
#include <QHostAddress>
#include <QRegularExpression>
#include "Logger.h"

namespace Utils {

QString parseIpAddress(const QByteArray &data, int offset)
{
    if (offset + 4 > data.size())
    {
        Logger::instance()->log(Logger::Warning, "Utils", QString("parseIpAddress: 数据越界, offset=%1, dataLen=%2").arg(offset).arg(data.size()));
        return "0.0.0.0";
    }
    
    quint8 b1 = static_cast<quint8>(data[offset]);
    quint8 b2 = static_cast<quint8>(data[offset + 1]);
    quint8 b3 = static_cast<quint8>(data[offset + 2]);
    quint8 b4 = static_cast<quint8>(data[offset + 3]);
    
    QString ip = QString("%1.%2.%3.%4").arg(b1).arg(b2).arg(b3).arg(b4);
    Logger::instance()->log(Logger::Debug, "Utils", QString("parseIpAddress: 解析IP地址: %1").arg(ip));
    return ip;
}

QByteArray ipStringToBytes(const QString &ipStr)
{
    QHostAddress ip(ipStr);
    QByteArray result;
    QStringList parts = ipStr.split('.');
    
    if (ip.isNull() || ip.protocol() != QAbstractSocket::IPv4Protocol)
    {
        Logger::instance()->log(Logger::Warning, "Utils", QString("ipStringToBytes: 无效的IP地址格式: %1").arg(ipStr));
        return QByteArray(4, 0);
    }
    
    // 2. 获取IPv4地址的数值形式
    quint32 ipv4 = ip.toIPv4Address();  // 返回的是主机字节序的quint32

    // 3. 转换为大端序（网络字节序）
    quint32 bigEndianIp = qToBigEndian(ipv4);

    // 4. 将4字节数据存入QByteArray
    result.resize(4);
    result[0] = (bigEndianIp >> 24) & 0xFF;
    result[1] = (bigEndianIp >> 16) & 0xFF;
    result[2] = (bigEndianIp >> 8) & 0xFF;
    result[3] = bigEndianIp & 0xFF;
    
    Logger::instance()->log(Logger::Debug, "Utils", QString("ipStringToBytes: 转换IP地址 %1 为字节数组").arg(ipStr));
    return result;
}

QString parseMacAddress(const QByteArray &data, int offset) {
    if (offset + 6 > data.size()) {
        Logger::instance()->log(Logger::Warning, "Utils", QString("parseMacAddress: 数据越界, offset=%1, dataLen=%2").arg(offset).arg(data.size()));
        return "00:00:00:00:00:00";
    }
    
    QString mac;
    for (int i = 0; i < 6; ++i) {
        if (i > 0) {
            mac += ":";
        }
        quint8 byte = static_cast<quint8>(data[offset + i]);
        mac += QString("%1").arg(byte, 2, 16, QChar('0')).toUpper();
    }
    
    Logger::instance()->log(Logger::Debug, "Utils", QString("parseMacAddress: 解析MAC地址: %1").arg(mac));
    return mac;
}

QByteArray macStringToBytes(const QString &mac) {
    QByteArray result;
    
    // 移除可能的分隔符
    QString cleanMac = mac;
    cleanMac.remove(':');
    cleanMac.remove('-');
    cleanMac.remove(' ');
    
    if (cleanMac.length() != 12) {
        Logger::instance()->log(Logger::Warning, "Utils", QString("macStringToBytes: 无效的MAC地址格式: %1").arg(mac));
        return QByteArray(6, 0);
    }
    
    for (int i = 0; i < 12; i += 2) {
        QString byteStr = cleanMac.mid(i, 2);
        bool ok;
        int value = byteStr.toInt(&ok, 16);
        if (!ok) {
            Logger::instance()->log(Logger::Warning, "Utils", QString("macStringToBytes: 无效的MAC地址字节: %1").arg(byteStr));
            return QByteArray(6, 0);
        }
        result.append(static_cast<char>(value));
    }
    
    Logger::instance()->log(Logger::Debug, "Utils", QString("macStringToBytes: 转换MAC地址 %1 为字节数组").arg(mac));
    return result;
}

int extractPortNumber(const QString &text) {
    // 使用正则表达式提取数字
    QRegularExpression re("\\d+");
    QRegularExpressionMatch match = re.match(text);
    
    if (match.hasMatch()) {
        bool ok;
        int port = match.captured(0).toInt(&ok);
        if (ok) {
            Logger::instance()->log(Logger::Debug, "Utils", QString("extractPortNumber: 从文本 %1 中提取端口号: %2").arg(text).arg(port));
            return port;
        }
    }
    
    Logger::instance()->log(Logger::Warning, "Utils", QString("extractPortNumber: 无法从文本中提取端口号: %1").arg(text));
    return -1;
}

} // namespace Utils
