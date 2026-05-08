#ifndef SERIALPORTCONFIG_H
#define SERIALPORTCONFIG_H

#include <QString>
#include <QVariantMap>
#include <QtGlobal>

/**
 * @brief 串口配置数据结构
 * 
 * 存储单个串口的完整配置信息,包括基本参数和工作模式
 */
struct SerialPortConfig {
    int index;              // 串口索引 (1-16)
    QString alias;          // 串口别名
    quint32 baudRate;       // 波特率
    quint8 dataBits;        // 数据位 (5,6,7,8)
    quint8 stopBits;        // 停止位 (1,2)
    quint8 parity;          // 校验位 (0:None, 1:Odd, 2:Even)
    quint8 flowControl;     // 流控
    quint8 fifo;            // FIFO
    quint8 interface;       // 接口类型 (RS232/RS422/RS485)
    QByteArray syncBitmap;
    // 工作模式相关
    quint8 workMode;        // 工作模式 (RealCOM/TCPServer/TCPClient/UDP)
    QVariantMap modeParams; // 模式特定参数
    
    // 默认构造函数
    SerialPortConfig()
        : index(0)
        , baudRate(9600)
        , dataBits(8)
        , stopBits(1)
        , parity(0)
        , flowControl(0)
        , fifo(0)
        , interface(0)
        , workMode(0)
    {}
};

struct SerialPortMode
{
    int index;  // 串口索引
    QString alias;  // 串口别名
    quint8 workMode;    // 串口工作模式
    quint8 TcpTime; // TCP存活检测时间
    quint8 TcpConMaxNum;    // TCP最大连接数
};

#endif // SERIALPORTCONFIG_H
