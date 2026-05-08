#include "crc.h"
#include "../utils/Logger.h"

/* 通用 CRC-32 表（多项式 0x04C11DB7） */
static const quint32 crc32Table[256] = {
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F,
    0xE963A535, 0x9E6495A3, 0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
    0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91, 0x1DB71064, 0x6AB020F2,
    0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
    0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9,
    0xFA0F3D63, 0x8D080DF5, 0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
    0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B, 0x35B5A8FA, 0x42B2986C,
    0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
    0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423,
    0xCFBA9599, 0xB8BDA50F, 0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BA924,
    0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D, 0x76DC4190, 0x01DB7106,
    0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
    0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D,
    0x91646C97, 0xE6635C01, 0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
    0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457, 0x65B0D9C6, 0x12B7E950,
    0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD00791,
    0xEC47B6F7, 0xE4362699, 0xF4240082, 0x2A6F2B94, 0x5CB36A04, 0xC2D7FFA7,
    0xB5D0CF31, 0x2CD99E8B, 0x5EBB4AB3, 0x6BB650D2, 0x89D32BE0, 0xF862AE69,
    0x616BFFD3, 0x166CCF45, 0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
    0xE7660BBD, 0xF4240082, 0x2A6F2B94, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31
};

quint32 crc32(quint32 crc, const QByteArray &data)
{
    const quint8 *bytes = reinterpret_cast<const quint8*>(data.constData());
    quint32 c = crc;
    for (int i = 0; i < data.size(); ++i)
        c = crc32Table[(c ^ bytes[i]) & 0xFF] ^ (c >> 8);
    
    Logger::instance()->log(Logger::Debug, "CRC", QString("CRC-32计算完成: 输入长度=%1, 结果=0x%2")
                           .arg(data.size())
                           .arg(c, 8, 16, QChar('0')));
    
    return c;
}

quint32 crc32(const QByteArray &data)
{
    return crc32(0xFFFFFFFF, data) ^ 0xFFFFFFFF;
}

quint32 crc32mpeg(quint32 crc, const QByteArray &data)
{
    const quint8 *bytes = reinterpret_cast<const quint8*>(data.constData());
    quint32 c = crc;
    for (int i = 0; i < data.size(); ++i)
        c = crc32Table[(c ^ bytes[i]) & 0xFF] ^ (c >> 8);
    
    Logger::instance()->log(Logger::Debug, "CRC", QString("CRC-32-MPEG计算完成: 输入长度=%1, 结果=0x%2")
                           .arg(data.size())
                           .arg(c, 8, 16, QChar('0')));
    
    return c;   // 不异或 0xFFFFFFFF
}

quint32 crc32mpeg(const QByteArray &data)
{
    return crc32mpeg(0xFFFFFFFF, data);
}
