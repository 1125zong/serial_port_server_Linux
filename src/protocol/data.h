#ifndef DATA_H
#define DATA_H

#include <QMainWindow>
#include <cstdint>   // 固定宽度整数类型（uint8_t等）
#include <QByteArray> // 可选，用于UDP数据包接收与解析

#include <QtGlobal>   // quint32 / quint8


enum class PendingOperation {
    None,
    FirmwareUpgrade,
    PackageUpgrade
};

#pragma pack(push, 1)   // 1 字节对齐，确保大小一致
struct fw_package_header_t
{
    quint32 magic_number;   // 0
        quint32 header_crc32;   // 4
        quint32 bit_length;     // 8
        quint32 app_length;     // 12
        quint32 bit_crc32;      // 16   ← 补上这一行
        quint32 app_crc32;      // 20   ← 如果还有应用 CRC，也补上
    // 后面紧跟 bitstream + application，头里不再放其他字段
};
#pragma pack(pop)

/* 魔数与大小常量 */
constexpr quint32 FW_PACKAGE_MAGIC = 0x55504454;
constexpr quint32 FW_PACKAGE_HEADER_SIZE = sizeof(fw_package_header_t);

/**
 * @brief 设备状态枚举（对应协议4.2节字段5，1Byte）
 */
enum class QPORT_DeviceState : uint8_t {
    FactoryMode = 0x01,  // 出厂模式
    WorkingMode = 0x02,  // 工作模式
    NetworkError = 0x03, // 网络故障
    SerialError = 0x04,  // 端口故障
    SystemError = 0x05   // 系统故障
};

/**
 * @brief 网口链接状态枚举（对应协议4.2节字段9、12，1Byte）
 */
enum class QPORT_LanLinkState : uint8_t {
    NoLink = 0x00,  // 无链接
    Linked = 0x01   // 已链接
};

/**
 * @brief LCD屏存在状态枚举（对应协议4.2节字段13，1Byte）
 */
enum class QPORT_LCDState : uint8_t {
    NoLCD = 0x00,   // 无LCD屏
    HasLCD = 0x01   // 有LCD屏
};

/**
 * @brief 电源状态枚举（对应协议4.2节字段14、15，1Byte）
 */
enum class QPORT_PowerState : uint8_t {
    PowerOff = 0x00, // 电源关闭
    PowerOn = 0x01   // 电源开启
};

/**
 * @brief 设备锁定状态枚举（对应协议4.2节字段18，1Byte）
 */
enum class QPORT_DeviceLockState : uint8_t
{
    Unlocked = 0x00, // 未锁定
    Locked = 0x01    // 已锁定（具体逻辑需结合业务）
};

enum class PortWorkMode : uint8_t {
    TcpServer = 0x02,  // TCP服务器模式
    RealCom = 0x01     // 虚拟端口模式
};

// 端口工作状态
enum class PortWorkStatus : uint8_t {
    Disconnected = 0x00,  // 未连接
    Connected = 0x01      // 已连接
};

// 端口锁定状态
enum class PortLockStatus : uint8_t {
    Unlocked = 0x00,  // 未锁定（可配置）
    Locked = 0x01     // 已锁定（配置不可修改）
};

//端口接口类型大类（对应协议4.2节字段10、11，1Byte）
enum class PortInterfaceTypeCategory : uint8_t {
    UART = 0x01,  // 端口
    CAN = 0x02   // CAN接口
    
};

// 端口接口类型（对应协议中接口类型细分）
enum class SerialPortInterfaceType : uint8_t {
    RS232 = 0x00,
    RS422 = 0x01,
    RS485 = 0x02,
    CAN = 0x03
};

// 端口校验位（对应端口参数）
enum class PortParity : uint8_t {
    None = 0x00,  // 'n' 无校验
    Odd = 0x10,   // 'o' 奇校验
    Even = 0x08   // 'e' 偶校验
};

enum class ProtocolFuncID : uint8_t {
    Func_Overview = 0x01,        // 概述（4.1节）：读取设备基础信息
    Func_BasicSetting = 0x02,    // 基础设置（4.2节）：配置设备名称等
    Func_NetworkSetting = 0x03,  // 网络设置（4.3节）：读写IP/子网掩码等
    Func_SerialSetting = 0x04,   // 端口设置（4.4节）：读写端口参数（波特率等）
    Func_SerialMode = 0x05,      // 端口模式设置（4.5节）：配置Real COM/TCP Server等模式
    Func_ReadSerialMode = 0x06,  // 读取端口模式参数（4.6节）：读取当前/指定模式参数
    Func_StatusMonitor = 0x07,   // 状态监控（4.7节）：读取连接状态/数据统计/错误信息
    Func_DeviceManage = 0x08,     // 设备管理（4.8节）：端口重置/锁定/看门狗/恢复出厂等
    Func_FirmwareUpgrade = 0xFF   //固件升级
};
/**
 * @brief 概述模块子功能ID（对应协议4.1节，1Byte）
 */
enum class OverviewSubFuncID : uint8_t {
    ReadOverviewInfo = 0x01  // 4.1.1：读取概述信息（设备型号/序列号等）
};

/**
 * @brief 网络设置模块子功能ID（对应协议4.3节，1Byte）
 */
enum class NetworkSubFuncID : uint8_t {
    ReadNetworkConfig = 0x01,  // 4.3.1：读取网络设置（IP/子网掩码等）
    WriteNetworkConfig = 0x02  // 4.3.2：配置网络设置
};

/**
 * @brief 端口设置模块子功能ID（对应协议4.4节，1Byte）
 */
enum class SerialSubFuncID : uint8_t {
    ReadSerialConfig = 0x01,   // 4.4.1：读取端口设置（单个/所有端口）
    WriteSingleSerial = 0x02,  // 4.4.2：写入指定端口设置
    WriteMultiSerial = 0x03    // 4.4.3：配置多端口
};

/**
 * @brief 设备管理模块子功能ID（对应协议4.8节，1Byte）
 */
enum class DeviceManageSubFuncID : uint8_t {
    ResetPort = 0x01,          // 4.8.1：重置端口
    LockPort = 0x02,           // 4.8.2：锁定端口
    UnlockPort = 0x03,         // 4.8.3：解锁端口
    QueryPortLock = 0x04,      // 4.8.4：查询端口锁定状态
    ConfigPortMirror = 0x05,   // 4.8.5：配置端口镜像
    LockDevice = 0x06,         // 4.8.6：锁定设备
    UnlockDevice = 0x07,       // 4.8.7：解锁设备
    SerialSelfTest = 0x08,     // 4.8.8：执行端口自检
    ConfigWatchdog = 0x09,     // 4.8.9：配置看门狗
    RestoreFactory = 0x0C,     // 4.8.10：恢复出厂设置
    SaveAndReboot = 0x0C       // 4.8.11：保存并重启（协议标注子功能ID同恢复出厂，需结合场景区分）
};
/*
 * @brief 通用数据帧结构体（对应协议3.2节，适用于所有TCP请求-应答帧）
 * @note 1. 帧头/帧尾为固定值，初始化时可默认赋值；
 *       2. 数据负载长度可变，通过QByteArray动态存储；
 *       3. 子功能ID需结合“功能ID”选择对应枚举（如Func_Overview对应OverviewSubFuncID）
 */
struct ProtocolGeneralFrame {
    // 1. 帧头（协议3.2节，2Byte，固定值0xA5A5）
    uint16_t frameHeader = 0xA5A5;  // 十六进制固定值，无需手动修改

    // 2. 功能ID（协议3.2节+4章，1Byte，枚举值）
    ProtocolFuncID funcID;

    // 3. 子功能ID（协议3.2节+4章，1Byte，需结合功能ID选择对应枚举）
    uint8_t subFuncID;  // 用uint8_t兼容所有子功能枚举（使用时强转对应枚举类型）

    // 4. 数据负载（协议3.2节，NByte可变长度）
    QByteArray dataPayload;  // 存储指令参数（请求帧）或返回数据（响应帧）

    // 5. 帧尾（协议3.2节，2Byte，固定值0x5A5A）
    uint16_t frameTail = 0x5A5A;    // 十六进制固定值，无需手动修改

    /*
     * @brief 辅助函数：将结构体转换为二进制数据流（用于TCP发送）
     * @return 完整的二进制帧（帧头+功能ID+子功能ID+负载+帧尾）
     */
    QByteArray toByteArray() const {
        QByteArray frame;
        // 写入帧头（大端序，协议默认字节序）
        frame.append(reinterpret_cast<const char*>(&frameHeader), sizeof(frameHeader));
        // 写入功能ID
        frame.append(reinterpret_cast<const char*>(&funcID), sizeof(funcID));
        // 写入子功能ID
        frame.append(reinterpret_cast<const char*>(&subFuncID), sizeof(subFuncID));
        // 写入数据负载
        frame.append(dataPayload);
        // 写入帧尾（大端序）
        frame.append(reinterpret_cast<const char*>(&frameTail), sizeof(frameTail));
        return frame;
    }

    /*
     * @brief 辅助函数：从二进制数据流解析到结构体（用于TCP接收）
     * @param data 接收的二进制数据流
     * @return true：解析成功；false：数据长度不足或帧头/帧尾错误
     */
    bool fromByteArray(const QByteArray& data) {
        // 校验最小长度：帧头(2)+功能ID(1)+子功能ID(1)+帧尾(2) = 6Byte
        if (data.size() < 6) {
            return false;
        }

        int offset = 0;
        // 解析帧头
        memcpy(&frameHeader, data.data() + offset, sizeof(frameHeader));
        offset += sizeof(frameHeader);
        if (frameHeader != 0xA5A5) {  // 帧头不匹配，解析失败
            return false;
        }

        // 解析功能ID
        memcpy(&funcID, data.data() + offset, sizeof(funcID));
        offset += sizeof(funcID);

        // 解析子功能ID
        memcpy(&subFuncID, data.data() + offset, sizeof(subFuncID));
        offset += sizeof(subFuncID);

        // 解析数据负载（总长度 - 帧头 - 功能ID - 子功能ID - 帧尾）
        uint32_t payloadLen = data.size() - offset - sizeof(frameTail);
        if (payloadLen > 0) {
            dataPayload = data.mid(offset, payloadLen);
        } else {
            dataPayload.clear();  // 无负载（如“读取概述信息”请求帧）
        }
        offset += payloadLen;

        // 解析帧尾
        memcpy(&frameTail, data.data() + offset, sizeof(frameTail));
        if (frameTail != 0x5A5A) {  // 帧尾不匹配，解析失败
            return false;
        }

        return true;
    }
};

/**
 * 单个端口状态结构体（新增）
 */
struct PortStatus {
    uint8_t portNum;               // 端口号（如1、2...最多16个）
    uint32_t baudRate;             // 波特率（如9600、115200）
    uint8_t dataBits;              // 数据位（5/6/7/8）
    PortParity parity;             // 校验位（n/o/e）
    uint8_t stopBits;              // 停止位（1/1.5/2）
    PortWorkMode workMode;         // 工作模式（TCP服务器/虚拟端口）
    PortWorkStatus workStatus;     // 连接状态（已连接/未连接）
    PortLockStatus lockStatus;     // 锁定状态（已锁定/未锁定）
    SerialPortInterfaceType ifType;      // 接口类型（RS232/RS485等）
};

struct DeviceInfo
{
    // 1. 设备型号（协议4.2字段1：String，8Byte）
    QString deviceModel;  // 示例："1255xyzp"（8字节有效内容，Qt自动管理字符串长度）

    // 2. 序列号（协议4.2字段2：String，8Byte，设唯一标识）
    QString serialNum;    // 示例："25125001"（格式：YYMDDNNN）

    // 3. 设备名称（协议4.2字段3：String，8Byte，可自定义）
    QString deviceName;   // 示例："25125001"（默认与序列号一致）

    // 4. 固件版本（协议4.2字段4：String，8Byte，格式VX.Y）
    QString firmwareVer;  // 示例："1001abcd"（备份标识格式：BBBB:BBBB）

    // 5. 设备状态（协议4.2字段5：1Byte，枚举值）
    QPORT_DeviceState deviceState;

    // 6. 网口有效数量（协议4.2字段6：1Byte，当前设备网口总数）
    uint8_t validLanCount;  // 示例：2（表示设备含2个网口）

    // 7. 网口1 IP地址（协议4.2字段7：格式XXX.XXX.XXX.XXX）
    QString lan1Ip;         // 示例："192.168.1.100"

    // 8. 网口1 MAC地址（协议4.2字段8：格式XX:XX:XX:XX:XX:XX）
    QString lan1Mac;        // 示例："00:1A:2B:3C:4D:5E"

    // 9. 网口1链接状态（协议4.2字段9：1Byte，枚举值）
    QPORT_LanLinkState lan1LinkState;

    // 10. 网口2 IP地址（协议4.2字段10：同网口1格式）
    QString lan2Ip;         // 示例："192.168.1.101"

    // 11. 网口2 MAC地址（协议4.2字段11：同网口1格式）
    QString lan2Mac;        // 示例："00:1A:2B:3C:4D:5F"

    // 12. 网口2链接状态（协议4.2字段12：1Byte，枚举值）
    QPORT_LanLinkState lan2LinkState;

    // 13. LCD屏状态（协议4.2字段13：1Byte，枚举值）
    QPORT_LCDState lcdState;

    // 14. 电源1状态（协议4.2字段14：1Byte，枚举值）
    QPORT_PowerState power1State;

    // 15. 电源2状态（协议4.2字段15：1Byte，枚举值）
    QPORT_PowerState power2State;

    // 16. 接口类型数量（协议4.2字段16：4Byte String，格式如"02030110"）
    QString ifTypeCount;    // 示例："02030110"（含义：0x02=CAN×3，0x01=UART×16）

    // 17. 接口状态（协议4.2字段17：16Byte，每1个Byte代表 1 个接口工作状态）
    uint8_t ifStatus[16];    // 示例：{0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00}（前32个接口正常）

    // 18. 设备锁定状态（协议4.2字段18：1Byte，枚举值）
    QPORT_DeviceLockState devLockState;

    // 19. 接口锁定状态（协议4.2字段19：16Byte，每字节表示1个接口锁定状态）
    uint8_t ifLockStatus[16];// 示例：{0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF}（后32个接口锁定）

    // 20. 端口工作状态 14Byte
    QList<PortStatus> ports;
};
struct PortConnInfo {
    int  portIndex;
    bool locked;
    QString modeText;
    QString connIPs;
};

struct PortStatusInfo {
    int portIndex;
    quint64 txCount;
    quint64 rxCount;
    quint64 txTotalCount;
    quint64 rxTotalCount;
};

struct PortErrorInfo {
    int portIndex;
    quint64 frameErr;
    quint64 parityErr;
    quint64 overrunErr;
    quint64 breakErr;
    quint64 totalErr() const { return frameErr + parityErr + overrunErr + breakErr; }
};
#endif // DATA_H
