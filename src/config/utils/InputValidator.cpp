#include "InputValidator.h"
#include <QRegularExpression>
#include <QStringList>

InputValidator::InputValidator()
{
}

InputValidator::ValidationResult InputValidator::validateIpAddress(const QString& ip)
{
    if (ip.isEmpty()) {
        return ValidationResult(false, "IP地址不能为空");
    }
    
    // 首先尝试IPv4
    if (isValidIPv4(ip)) {
        return ValidationResult(true, "");
    }
    
    // 尝试IPv6
    if (isValidIPv6(ip)) {
        return ValidationResult(true, "");
    }
    
    return ValidationResult(false, "无效的IP地址格式");
}

InputValidator::ValidationResult InputValidator::validatePortNumber(int port)
{
    if (port < 1 || port > 65535) {
        return ValidationResult(false, QString("端口号必须在1到65535之间，得到 %1").arg(port));
    }
    return ValidationResult(true, "");
}

InputValidator::ValidationResult InputValidator::validateMinorNumber(int minor)
{
    if (minor < 0 || minor > 255) {
        return ValidationResult(false, QString("次设备号必须在0到255之间，得到 %1").arg(minor));
    }
    return ValidationResult(true, "");
}

InputValidator::ValidationResult InputValidator::validateTtyName(const QString& ttyName)
{
    if (ttyName.isEmpty()) {
        return ValidationResult(false, "TTY名称不能为空");
    }
    
    // TTY名称格式: ttyw后跟2位十六进制字符 (例如: ttyw00, ttyw0a, ttyw9f)
    QRegularExpression ttyRegex("^ttyw[0-9a-fA-F]{2}$");
    if (!ttyRegex.match(ttyName).hasMatch()) {
        return ValidationResult(false, QString("无效的TTY名称格式: %1 (预期格式: ttywXX，其中XX是十六进制字符)").arg(ttyName));
    }
    
    return ValidationResult(true, "");
}

InputValidator::ValidationResult InputValidator::validatePortCount(int count)
{
    QList<int> validCounts = {1, 4, 8, 16};
    if (!validCounts.contains(count)) {
        return ValidationResult(false, QString("端口数量必须是1、4、8或16，得到 %1").arg(count));
    }
    return ValidationResult(true, "");
}

bool InputValidator::isValidIPv4(const QString& ip)
{
    QStringList parts = ip.split('.');
    if (parts.size() != 4) {
        return false;
    }
    
    for (const QString& part : parts) {
        bool ok;
        int value = part.toInt(&ok);
        if (!ok || value < 0 || value > 255) {
            return false;
        }
    }
    
    return true;
}

bool InputValidator::isValidIPv6(const QString& ip)
{
    // 简化的IPv6验证
    // 完整格式: 8组4位十六进制数字，以冒号分隔
    // 压缩格式: 允许使用::表示连续的零
    
    if (ip.contains(":::")) {
        return false; // 无效: 连续冒号超过两个
    }
    
    QStringList parts = ip.split(':');
    
    // 检查压缩格式 (::)
    bool hasCompression = ip.contains("::");
    
    if (hasCompression) {
        // 压缩格式下，可以少于8部分
        if (parts.size() > 8) {
            return false;
        }
    } else {
        // 非压缩格式下，必须正好有8部分
        if (parts.size() != 8) {
            return false;
        }
    }
    
    // 验证每个部分（十六进制数字，最多4个字符）
    QRegularExpression hexRegex("^[0-9a-fA-F]{1,4}$");
    for (const QString& part : parts) {
        if (part.isEmpty() && hasCompression) {
            continue; // 压缩格式下允许空部分
        }
        if (!hexRegex.match(part).hasMatch()) {
            return false;
        }
    }
    
    return true;
}
