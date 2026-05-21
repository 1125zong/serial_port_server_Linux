#ifndef UDPDISCOVERY_H
#define UDPDISCOVERY_H

#include <QObject>
#include <QUdpSocket>
#include <QTimer>
#include <QHostAddress>
#include <QNetworkInterface>
#include "../utils/Logger.h"
#include "../protocol/data.h"

/**
 * @brief UDP设备发现组件
 * 
 * 负责在局域网中通过UDP广播发现端口服务器设备
 * 实现Requirements 9.1-9.5中的设备发现功能
 * 使用单例模式确保整个应用只有一个实例
 */
class UdpDiscovery : public QObject {
    Q_OBJECT

public:
    static DeviceInfo dev;
    
    /**
     * @brief 获取单例实例
     */
    static UdpDiscovery* instance(QObject *parent = nullptr);
    
    ~UdpDiscovery();

    /**
     * @brief 开始搜索设备
     * 
     * 发送UDP广播包到局域网，等待设备响应
     */
    void startSearch();

    /**
     * @brief 停止搜索
     * 
     * 停止监听UDP响应
     */
    void stopSearch();

    /**
     * @brief 检查是否正在搜索
     */
    bool isSearching() const { return m_isSearching; }

 signals:
    /**
     * @brief 发现设备信号
     * @param device 发现的设备信息
     */
    void deviceDiscovered(const DeviceInfo &device);

    /**
     * @brief 搜索完成信号
     * @param deviceCount 发现的设备数量
     */
    void searchCompleted(int deviceCount);

    /**
     * @brief 搜索错误信号
     * @param error 错误描述
     */
    void searchError(const QString &error);

    void bindSucceeded(quint16 port);

private slots:
    void onReadyRead();
    void onSearchTimeout();

private:
    explicit UdpDiscovery(QObject *parent = nullptr);
    bool parseDiscoveryResponse(const QByteArray &data, const QHostAddress &sender, DeviceInfo &device);
    bool parseDiscoveryResponseBinary(const QByteArray &dg, const QHostAddress &sender, DeviceInfo &device);
    static QList<QHostAddress> getLocalAddresses();
    
    static UdpDiscovery* m_instance;
    QUdpSocket *m_socket;
    QTimer *m_searchTimer;
    Logger *m_logger;
    int m_deviceCount;
    bool m_isSearching;

    static constexpr quint16 UDP_PORT = 48899;
    static constexpr int SEARCH_TIMEOUT = 3000;  // 搜索超时(ms)
};

#endif // UDPDISCOVERY_H
