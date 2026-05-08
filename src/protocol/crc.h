#ifndef CRC_H
#define CRC_H

#include <QtGlobal>   // quint32 / quint8
#include <QByteArray>

/* 通用 CRC-32（多项式 0x04C11DB7，初始值 0xFFFFFFFF，异或 0xFFFFFFFF） */
quint32 crc32(quint32 crc, const QByteArray &data);
quint32 crc32(const QByteArray &data); // 从 0xFFFFFFFF 开始

/* 固件包头用的 CRC-32-MPEG2（多项式 0x04C11DB7，初始值 0xFFFFFFFF，异或 0） */
quint32 crc32mpeg(quint32 crc, const QByteArray &data);
quint32 crc32mpeg(const QByteArray &data);

#endif // CRC_H
