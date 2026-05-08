#ifndef UTILS_H
#define UTILS_H

#include <limits>
#include <QString>
#include <QByteArray>
#include <QtEndian>
#include <QDebug>

namespace Utils {
    // IP地址转换
    QString parseIpAddress(const QByteArray &data, int offset);
    QByteArray ipStringToBytes(const QString &ip);
    
    // MAC地址转换
    QString parseMacAddress(const QByteArray &data, int offset);
    QByteArray macStringToBytes(const QString &mac);
    
    // 端口号提取
    int extractPortNumber(const QString &text);
    
    // 字节序转换(使用Qt函数)
    template<typename T>
    T fromBigEndian(const QByteArray &data, int offset) {
        if (offset + static_cast<int>(sizeof(T)) > data.size()) {
            qWarning() << "数据越界: offset=" << offset 
                       << "size=" << sizeof(T) 
                       << "dataLen=" << data.size();
            return T();
        }
        return qFromBigEndian<T>(reinterpret_cast<const uchar*>(data.constData() + offset));
    }
    
    template<typename T>
    QByteArray toBigEndian(T value) {
        QByteArray result(sizeof(T), 0);
        qToBigEndian(value, reinterpret_cast<uchar*>(result.data()));
        return result;
    }
    
    // 安全读取
    template<typename T>
    T safeRead(const QByteArray &data, int offset, T defaultValue = T())
    {
        if (offset + static_cast<int>(sizeof(T)) > data.size()) {
            qWarning() << "数据越界: offset=" << offset 
                       << "size=" << sizeof(T) 
                       << "dataLen=" << data.size();
            return defaultValue;
        }
        return *reinterpret_cast<const T*>(data.constData() + offset);
    }
}

#endif // UTILS_H
