#include "FrameValidator.h"
#include "../utils/ProtocolConstants.h"
#include "../utils/Logger.h"
#include <QRegularExpression>
#include <QtEndian>

bool FrameValidator::validateFrame(const QByteArray &frame) {
    // 检查最小长度
    if (!validateLength(frame, ProtocolConstants::MIN_FRAME_LENGTH)) {
        Logger::instance()->log(Logger::Warning, "FrameValidator", 
            QString("帧长度不足: %1 < %2").arg(frame.size()).arg(ProtocolConstants::MIN_FRAME_LENGTH));
        return false;
    }
    
    // 检查帧头
    quint16 header = qFromBigEndian<quint16>(reinterpret_cast<const uchar*>(frame.constData()));
    if (header != ProtocolConstants::FRAME_HEADER) {
        Logger::instance()->log(Logger::Warning, "FrameValidator", 
            QString("帧头错误: 0x%1 != 0x%2")
                .arg(header, 4, 16, QChar('0'))
                .arg(ProtocolConstants::FRAME_HEADER, 4, 16, QChar('0')));
        return false;
    }
    
    // 检查帧尾
    quint16 tail = qFromBigEndian<quint16>(
        reinterpret_cast<const uchar*>(frame.constData() + frame.size() - 2));
    if (tail != ProtocolConstants::FRAME_TAIL) {
        Logger::instance()->log(Logger::Warning, "FrameValidator", 
            QString("帧尾错误: 0x%1 != 0x%2")
                .arg(tail, 4, 16, QChar('0'))
                .arg(ProtocolConstants::FRAME_TAIL, 4, 16, QChar('0')));
        return false;
    }
    
    Logger::instance()->log(Logger::Info, "FrameValidator", 
        QString("帧验证成功，长度: %1").arg(frame.size()));
    return true;
}

bool FrameValidator::validateLength(const QByteArray &data, int expectedMin, int expectedMax) {
    int length = data.size();
    
    // 检查最小长度
    if (length < expectedMin) {
        Logger::instance()->log(Logger::Warning, "FrameValidator", 
            QString("数据长度不足: %1 < %2").arg(length).arg(expectedMin));
        return false;
    }
    
    // 检查最大长度 (如果指定)
    if (expectedMax > 0 && length > expectedMax) {
        Logger::instance()->log(Logger::Warning, "FrameValidator", 
            QString("数据长度超出: %1 > %2").arg(length).arg(expectedMax));
        return false;
    }
    
    Logger::instance()->log(Logger::Debug, "FrameValidator", 
        QString("数据长度验证成功: %1").arg(length));
    return true;
}

bool FrameValidator::validateIpAddress(const QString &ip) {
    // IP地址格式: XXX.XXX.XXX.XXX (每部分0-255)
    QRegularExpression ipRegex("^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$");
    
    if (!ipRegex.match(ip).hasMatch()) {
        Logger::instance()->log(Logger::Warning, "FrameValidator", 
            QString("IP地址格式错误: %1").arg(ip));
        return false;
    }
    
    Logger::instance()->log(Logger::Debug, "FrameValidator", 
        QString("IP地址验证成功: %1").arg(ip));
    return true;
}

bool FrameValidator::validateMacAddress(const QString &mac) {
    // MAC地址格式: XX:XX:XX:XX:XX:XX 或 XX-XX-XX-XX-XX-XX
    QRegularExpression macRegex("^([0-9A-Fa-f]{2}[:-]){5}([0-9A-Fa-f]{2})$");
    
    if (!macRegex.match(mac).hasMatch()) {
        Logger::instance()->log(Logger::Warning, "FrameValidator", 
            QString("MAC地址格式错误: %1").arg(mac));
        return false;
    }
    
    Logger::instance()->log(Logger::Debug, "FrameValidator", 
        QString("MAC地址验证成功: %1").arg(mac));
    return true;
}

bool FrameValidator::validatePortNumber(int port, int min, int max) {
    if (port < min || port > max) {
        Logger::instance()->log(Logger::Warning, "FrameValidator", 
            QString("端口号超出范围: %1 不在 [%2, %3]").arg(port).arg(min).arg(max));
        return false;
    }
    
    Logger::instance()->log(Logger::Debug, "FrameValidator", 
        QString("端口号验证成功: %1").arg(port));
    return true;
}

bool FrameValidator::validateBaudRate(quint32 baudRate) {
    // 常见波特率列表
    QList<quint32> validBaudRates = {
        300, 600, 1200, 2400, 4800, 9600, 14400, 19200, 
        38400, 57600, 115200, 230400, 460800, 921600
    };
    
    if (!validBaudRates.contains(baudRate)) {
        Logger::instance()->log(Logger::Warning, "FrameValidator", 
            QString("波特率不在标准列表中: %1").arg(baudRate));
        return false;
    }
    
    Logger::instance()->log(Logger::Debug, "FrameValidator", 
        QString("波特率验证成功: %1").arg(baudRate));
    return true;
}
