#ifndef NETWORKCONFIG_H
#define NETWORKCONFIG_H

#include <QString>
#include <QtGlobal>

/**
 * @brief 网络配置数据结构
 * 
 * 存储设备的网络配置信息,包括双网口配置和DNS设置
 */
struct NetworkConfig {
    quint8 lanCount;        // 网口数量
    
    // LAN1 配置
    bool lan1UseDhcp;       // 是否使用DHCP
    QString lan1Ip;
    QString lan1Netmask;
    QString lan1Gateway;
    quint8 lan1Speed;       // 速度设置
    
    // LAN2 配置
    bool lan2UseDhcp;
    QString lan2Ip;
    QString lan2Netmask;
    QString lan2Gateway;
    quint8 lan2Speed;
    
    // 默认构造函数
    NetworkConfig()
        : lanCount(0)
        , lan1UseDhcp(false)
        , lan1Speed(0)
        , lan2UseDhcp(false)
        , lan2Speed(0)
    {}
};

#endif // NETWORKCONFIG_H
