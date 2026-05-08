#include "TtyGenerator.h"
#include <QRegularExpression>

const QString TtyGenerator::TTY_PREFIX = "ttyr";
const QString TtyGenerator::COUT_PREFIX = "cur";

TtyGenerator::TtyGenerator()
{
}

QString TtyGenerator::generateTtyName(int minor)
{
    // 格式: ttyr + 2位minor编号 (例如: ttyr00, ttyr01, ttyr99)
    return QString("%1%2").arg(TTY_PREFIX).arg(minor, 2, 10, QChar('0'));
}

QString TtyGenerator::generateCoutName(const QString& ttyName)
{
    // 从TTY名称中提取数字部分并用于cout名称
    // 例如: "ttyr00" -> "cur00"
    
    if (!isValidTtyName(ttyName)) {
        return QString(); // 对于无效输入返回空字符串
    }
    
    // 从TTY名称中提取2位数字
    QString numericPart = ttyName.mid(TTY_PREFIX.length());
    
    return QString("%1%2").arg(COUT_PREFIX).arg(numericPart);
}

int TtyGenerator::getMinorFromTtyName(const QString& ttyName)
{
    if (!isValidTtyName(ttyName)) {
        return -1;
    }
    
    // 提取数字部分
    QString numericPart = ttyName.mid(TTY_PREFIX.length());
    
    bool ok;
    int minor = numericPart.toInt(&ok);
    
    if (!ok || minor < 0 || minor > 99) {
        return -1;
    }
    
    return minor;
}

bool TtyGenerator::isValidTtyName(const QString& ttyName)
{
    // TTY名称格式: ttyr或ttyw后跟2位十六进制字符
    QRegularExpression ttyRegex("^ttyr[0-9a-fA-F]{2}$|^ttyw[0-9a-fA-F]{2}$");
    return ttyRegex.match(ttyName).hasMatch();
}
