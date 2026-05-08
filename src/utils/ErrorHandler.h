#ifndef ERRORHANDLER_H
#define ERRORHANDLER_H

#include <QString>
#include <QMessageBox>
#include <QWidget>
#include "Logger.h"

/**
 * @brief 错误类型枚举
 * 
 * 定义系统中所有可能的错误类型
 */
enum class ErrorType {
    // 网络错误
    ConnectionTimeout,
    ConnectionFailed,
    ConnectionLost,
    SendFailed,
    
    // 协议错误
    InvalidFrame,
    InvalidFrameHeader,
    InvalidFrameTail,
    InvalidLength,
    ParseFailed,
    
    // 数据错误
    InvalidData,
    DataOutOfRange,
    BufferOverflow,
    
    // 设备错误
    DeviceLocked,
    PortLocked,
    OperationFailed,
    DeviceNotResponding,
    
    // 系统错误
    FileIOError,
    ConfigError,
    UnknownError
};

/**
 * @brief 错误处理器
 * 
 * 提供统一的错误处理、恢复和用户通知机制
 * 实现Requirements 8.1-8.5, 9.1-9.5
 */
class ErrorHandler {
public:
    /**
     * @brief 处理错误
     * @param type 错误类型
     * @param context 错误上下文信息
     * @param parent 父窗口（用于显示消息框）
     */
    static void handleError(ErrorType type, const QString &context, QWidget *parent = nullptr);
    
    /**
     * @brief 尝试错误恢复
     * @param type 错误类型
     * @return 是否成功恢复
     */
    static bool tryRecover(ErrorType type);
    
    /**
     * @brief 通知用户错误
     * @param type 错误类型
     * @param message 错误消息
     * @param parent 父窗口
     */
    static void notifyUser(ErrorType type, const QString &message, QWidget *parent = nullptr);
    
    /**
     * @brief 获取错误类型的字符串描述
     * @param type 错误类型
     * @return 错误类型字符串
     */
    static QString errorTypeToString(ErrorType type);
    
    /**
     * @brief 获取错误类型对应的图标
     * @param type 错误类型
     * @return 消息框图标
     */
    static QMessageBox::Icon errorTypeToIcon(ErrorType type);
    
    /**
     * @brief 获取错误类型的用户友好消息
     * @param type 错误类型
     * @return 用户友好的错误描述
     */
    static QString getUserFriendlyMessage(ErrorType type);
    
    /**
     * @brief 获取错误类型的建议操作
     * @param type 错误类型
     * @return 建议用户采取的操作
     */
    static QString getSuggestedAction(ErrorType type);
    
    /**
     * @brief 判断错误是否可恢复
     * @param type 错误类型
     * @return 是否可恢复
     */
    static bool isRecoverable(ErrorType type);
    
    /**
     * @brief 判断错误是否需要立即通知用户
     * @param type 错误类型
     * @return 是否需要立即通知
     */
    static bool requiresImmediateNotification(ErrorType type);

private:
    ErrorHandler() = delete;  // 禁止实例化
    ~ErrorHandler() = delete;
};

#endif // ERRORHANDLER_H
