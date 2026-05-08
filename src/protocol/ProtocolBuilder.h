#ifndef PROTOCOLBUILDER_H
#define PROTOCOLBUILDER_H

#include <QByteArray>
#include <QString>
#include <QList>
#include <QVariantMap>
#include <QCheckBox>
#include <QRegularExpression>
#include <QWidget>
#include "../model/SerialPortConfig.h"
#include "../model/NetworkConfig.h"
#include "data.h"
#include <QNetworkInterface>        //2026.1.26 -- 获取mac地址

/**
 * @brief 协议帧构建器
 * 
 * 负责构建发送给设备的命令帧
 * 所有方法都是静态方法,无需实例化
 */
class  ProtocolBuilder {
public:
    // 构建概述查询命令
    static QByteArray buildOverviewQuery();
    
    //2026.01.26新增---读取上位机的MAC地址
    static QByteArray getMacAddress();

    // 构建网络配置命令
    static QByteArray buildNetworkConfigQuery();
    static QByteArray buildNetworkConfigWrite(const NetworkConfig &config);
    
    // 构建串口配置命令
    static QByteArray buildSerialConfigQuery(int portIndex = 0xFFFF);
    static QByteArray buildSerialConfigWrite(const SerialPortConfig &config);
    static QByteArray buildSerialConfigWrite1(const SerialPortConfig &config);
    static QByteArray buildMultiSerialConfigWrite(const QList<SerialPortConfig> &configs);
    
    // 构建模式设置命令
    static QByteArray buildModeConfigWrite(int portIndex, quint8 mode, const QVariantMap &params);
    static QByteArray buildModeConfigWrite1(quint8 mode, const QVariantMap &params,const SerialPortConfig &config);
    // 构建设备管理命令
    static QByteArray buildPortReset(quint16 portMask);
    static QByteArray buildPortLock(int portIndex);
    static QByteArray buildPortUnlock(int portIndex);
    static QByteArray buildPortLockStatusQuery();
    static QByteArray buildDeviceLock();
    static QByteArray buildDeviceUnlock(const QString &code);
    static QByteArray buildDeviceNameWrite(const QString &name);
    static QByteArray buildFactoryReset();
    static QByteArray buildReboot();
    static QByteArray buildSelfCheck(quint8 portByte);
    static QByteArray buildDeviceManageCommand(quint8 subFunc, const QByteArray &pld);
    static QByteArray buildStatusCommand(quint8 subFunc, const QByteArray &pld = QByteArray());
    static QByteArray buildSelfCheckWithSync(const QByteArray &syncBitmap);
    static QByteArray buildSelfCheckWithSync1(const QByteArray &syncBitmap);
    static QByteArray buildMultiPortLock(const QByteArray &bitmap, bool lock);
    static QByteArray buildFinalCheck();
    static QByteArray buildPortQuery(const int portIndex);
    static QByteArray buildPortSpecifiedMode(const QByteArray &mode);

private:
    // 构建完整帧
    static QByteArray buildFrame(quint8 funcId, quint8 subFuncId, const QByteArray &payload);
    
    // IP地址转换
    static QByteArray ipStringToBytes(const QString &ip);
    
    // MAC地址转换
    static QByteArray macStringToBytes(const QString &mac);
};

#endif // PROTOCOLBUILDER_H
