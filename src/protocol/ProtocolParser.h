#ifndef PROTOCOLPARSER_H
#define PROTOCOLPARSER_H

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QList>
#include <QMap>
#include "../model/SerialPortConfig.h"
#include "../model/NetworkConfig.h"
#include "data.h"

/**
 * @brief 端口锁定信息结构
 */
struct PortLockInfo {
    int portNum;           // 端口号
    bool lockState;      // 锁定状态
    QString lockMac;       // 锁定MAC地址
};

/**
 * @brief 端口状态信息结构
 */
//struct PortStatusInfo {
//    int portIndex;         // 端口索引
//    quint32 txCount;       // 发送计数
//    quint32 rxCount;       // 接收计数
//    quint64 txTotalCount;  // 总发送计数
//    quint64 rxTotalCount;  // 总接收计数
//};

/**
 * @brief 协议解析器
 * 
 * 负责将二进制协议数据转换为结构化数据对象
 * 实现了帧验证和各种响应类型的解析
 */
class ProtocolParser : public QObject {
    Q_OBJECT
    
public:
    explicit ProtocolParser(QObject *parent = nullptr);
    
    // 解析完整协议帧
    bool parseFrame(const QByteArray &frame);
    
    // 解析特定类型的响应
    bool parseOverviewResponse(const QByteArray &payload, DeviceInfo &info);
    bool parseNetworkConfigResponse(const QByteArray &payload, NetworkConfig &config);
    bool parseSerialConfigResponse(const QByteArray &payload, QList<SerialPortConfig> &configs);
    bool parsePortLockStatusResponse(const QByteArray &payload,
                                     QList<PortLockInfo> &lockStatus);
    bool parseStatusMonitorResponse(const QByteArray &payload, QList<PortStatusInfo> &status);
    bool validateFrame(const QByteArray &frame);
    bool parsePortConnInfoResponse(const QByteArray &payload, QList<PortConnInfo> &list);
    bool parsePortStatusResponse(const QByteArray &payload, QList<PortStatusInfo> &list);
    bool parsePortErrorResponse(const QByteArray &payload, QList<PortErrorInfo> &list);
signals:
    void overviewParsed(const DeviceInfo &info);
    void portLockStatusParsed(const QList<PortLockInfo> &);
    void statusMonitorParsed(const QList<PortStatusInfo> &status);
    void parseError(const QString &error);
    
private:

    bool checkFrameHeader(const QByteArray &frame);
    bool checkFrameTail(const QByteArray &frame);
};

#endif // PROTOCOLPARSER_H
