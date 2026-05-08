#ifndef FRAMEVALIDATOR_H
#define FRAMEVALIDATOR_H

#include <QByteArray>
#include <QString>
#include <QtGlobal>

/**
 * @brief 帧校验器
 * 
 * 提供数据验证功能
 * 所有方法都是静态方法,无需实例化
 */
class FrameValidator {
public:
    // 验证帧完整性
    static bool validateFrame(const QByteArray &frame);
    
    // 验证数据长度
    static bool validateLength(const QByteArray &data, int expectedMin, int expectedMax = -1);
    
    // 验证IP地址格式
    static bool validateIpAddress(const QString &ip);
    
    // 验证MAC地址格式
    static bool validateMacAddress(const QString &mac);
    
    // 验证端口号范围
    static bool validatePortNumber(int port, int min = 1, int max = 16);
    
    // 验证波特率
    static bool validateBaudRate(quint32 baudRate);
};

#endif // FRAMEVALIDATOR_H
