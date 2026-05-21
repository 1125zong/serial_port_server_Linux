#ifndef PROTOCOLCONSTANTS_H
#define PROTOCOLCONSTANTS_H

#include <QtGlobal>

namespace ProtocolConstants {
    // 帧标识
    constexpr quint16 FRAME_HEADER = 0xA5A5;
    constexpr quint16 FRAME_TAIL = 0x5A5A;
    
    // 帧最小长度
    constexpr int MIN_FRAME_LENGTH = 6;  // 帧头(2) + 功能ID(1) + 子功能ID(1) + 帧尾(2)
    
    // 超时设置
    constexpr int CONNECT_TIMEOUT_MS = 10000;
    constexpr int RESPONSE_TIMEOUT_MS = 5000;
    constexpr int SEARCH_TIMEOUT_MS = 5000;
    
    // 重连设置
    constexpr int MAX_RECONNECT_ATTEMPTS = 5;
    constexpr int RECONNECT_INTERVAL_MS = 3000;
    
    // 缓冲区大小
    constexpr int MAX_RECV_BUFFER_SIZE = 65536;
    
    // UDP设置
    constexpr quint16 UDP_PORT = 48899;
    constexpr char UDP_SEARCH_MSG[] = "SEARCH_DEVICE_WQ";
}

namespace OverviewOffset {
    // 设备概览响应偏移量
    constexpr int DEVICE_MODEL = 0;
    constexpr int DEVICE_MODEL_LEN = 40;
    
    constexpr int SERIAL_NUM = 40;
    constexpr int SERIAL_NUM_LEN = 8;
    
    constexpr int DEVICE_NAME = 48;
    constexpr int DEVICE_NAME_LEN = 40;
    
    constexpr int FIRMWARE_VER = 88;
    constexpr int FIRMWARE_VER_LEN = 4;
    
    constexpr int DEVICE_STATE = 92;
    constexpr int VALID_LAN_COUNT = 93;
    
    constexpr int LAN1_IP = 94;
    constexpr int LAN1_MAC = 97;
    constexpr int LAN1_SPEED = 104;
    
    constexpr int LAN2_IP = 105;
    constexpr int LAN2_MAC = 108;
    constexpr int LAN2_SPEED = 115;
    
    constexpr int LCD_STATE = 116;
    constexpr int POWER1_STATE = 117;
    constexpr int POWER2_STATE = 118;
    
    constexpr int IF_TYPE_COUNT = 118;
    constexpr int IF_STATUS = 120;
    constexpr int DEV_LOCK_STATE = 124;
    constexpr int IF_LOCK_STATUS = 125;
}

namespace SerialConfigOffset {
    // 端口配置块偏移量
    constexpr int PORT_INDEX = 0;
    constexpr int ALIAS_LEN = 1;
    constexpr int ALIAS_MAX_LEN = 19;
    constexpr int BAUD_RATE = 21;
    constexpr int DATA_BITS = 25;
    constexpr int STOP_BITS = 26;
    constexpr int PARITY = 27;
    constexpr int FLOW_CTRL = 28;
    constexpr int INTERFACE = 29;
    
    // 单个配置块总长度
    constexpr int BLOCK_SIZE = 30;
}

namespace PortLockOffset {
    // 端口锁定状态块偏移量
    constexpr int PORT_NUM = 0;
    constexpr int LOCK_STATE = 1;
    constexpr int LOCK_MAC = 2;
    constexpr int LOCK_MAC_LEN = 11;
    
    // 单个状态块总长度
    constexpr int BLOCK_SIZE = 13;  // 1 + 1 + 11
}

namespace StatusMonitorOffset {
    // 状态监控块偏移量
    constexpr int PORT_INDEX = 0;
    constexpr int TX_CNT = 1;
    constexpr int RX_CNT = 5;
    constexpr int TX_TOTAL_CNT = 9;
    constexpr int RX_TOTAL_CNT = 17;
    
    // 单个状态块总长度
    constexpr int BLOCK_SIZE = 25;  // 1 + 4 + 4 + 8 + 8
}

namespace ErrorMonitorOffset {
    // 错误监控块偏移量
    constexpr int PORT_INDEX = 0;
    constexpr int FRAME_ERROR = 1;
    constexpr int PARITY_ERROR = 5;
    constexpr int OVERRUN_ERROR = 9;
    constexpr int BREAK_EVENT = 13;
    
    // 单个错误块总长度
    constexpr int BLOCK_SIZE = 17;  // 1 + 4 + 4 + 4 + 4
}

#endif // PROTOCOLCONSTANTS_H
