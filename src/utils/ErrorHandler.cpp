#include "ErrorHandler.h"

void ErrorHandler::handleError(ErrorType type, const QString &context, QWidget *parent)
{
    // 记录错误日志
    Logger *logger = Logger::instance();
    QString errorMsg = QString("%1: %2").arg(errorTypeToString(type)).arg(context);
    logger->log(Logger::Error, "ErrorHandler", errorMsg);
    
    // 尝试恢复
    bool recovered = tryRecover(type);
    
    // 如果无法恢复或需要立即通知，则通知用户
    if (!recovered || requiresImmediateNotification(type)) {
        notifyUser(type, context, parent);
    }
}

bool ErrorHandler::tryRecover(ErrorType type)
{
    Logger *logger = Logger::instance();
    
    switch (type) {
    case ErrorType::ConnectionTimeout:
    case ErrorType::ConnectionFailed:
        // 连接错误由ConnectionManager自动重连处理
        logger->log(Logger::Info, "ErrorHandler", "Connection error will be handled by auto-reconnect");
        return true;  // 标记为可恢复（由自动重连处理）
        
    case ErrorType::BufferOverflow:
        // 缓冲区溢出已在TcpClient中清理
        logger->log(Logger::Info, "ErrorHandler", "Buffer overflow cleared");
        return true;
        
    case ErrorType::InvalidFrame:
    case ErrorType::InvalidFrameHeader:
    case ErrorType::InvalidFrameTail:
    case ErrorType::InvalidLength:
        // 无效帧已被丢弃
        logger->log(Logger::Info, "ErrorHandler", "Invalid frame discarded");
        return true;
        
    case ErrorType::ConnectionLost:
    case ErrorType::SendFailed:
    case ErrorType::ParseFailed:
    case ErrorType::InvalidData:
    case ErrorType::DataOutOfRange:
    case ErrorType::DeviceLocked:
    case ErrorType::PortLocked:
    case ErrorType::OperationFailed:
    case ErrorType::DeviceNotResponding:
    case ErrorType::FileIOError:
    case ErrorType::ConfigError:
    case ErrorType::UnknownError:
        // 这些错误无法自动恢复
        return false;
        
    default:
        return false;
    }
}

void ErrorHandler::notifyUser(ErrorType type, const QString &message, QWidget *parent)
{
    QString title = errorTypeToString(type);
    QString userMessage = getUserFriendlyMessage(type);
    QString suggestedAction = getSuggestedAction(type);
    
    // 组合完整消息
    QString fullMessage = userMessage;
    if (!message.isEmpty()) {
        fullMessage += "\n\n详细信息: " + message;
    }
    if (!suggestedAction.isEmpty()) {
        fullMessage += "\n\n建议: " + suggestedAction;
    }
    
    // 显示消息框
    QMessageBox::Icon icon = errorTypeToIcon(type);
    QMessageBox msgBox(parent);
    msgBox.setIcon(icon);
    msgBox.setWindowTitle(title);
    msgBox.setText(fullMessage);
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.exec();
}

QString ErrorHandler::errorTypeToString(ErrorType type)
{
    switch (type) {
    // 网络错误
    case ErrorType::ConnectionTimeout:
        return "连接超时";
    case ErrorType::ConnectionFailed:
        return "连接失败";
    case ErrorType::ConnectionLost:
        return "连接断开";
    case ErrorType::SendFailed:
        return "发送失败";
        
    // 协议错误
    case ErrorType::InvalidFrame:
        return "无效帧";
    case ErrorType::InvalidFrameHeader:
        return "无效帧头";
    case ErrorType::InvalidFrameTail:
        return "无效帧尾";
    case ErrorType::InvalidLength:
        return "无效长度";
    case ErrorType::ParseFailed:
        return "解析失败";
        
    // 数据错误
    case ErrorType::InvalidData:
        return "无效数据";
    case ErrorType::DataOutOfRange:
        return "数据越界";
    case ErrorType::BufferOverflow:
        return "缓冲区溢出";
        
    // 设备错误
    case ErrorType::DeviceLocked:
        return "设备已锁定";
    case ErrorType::PortLocked:
        return "端口已锁定";
    case ErrorType::OperationFailed:
        return "操作失败";
    case ErrorType::DeviceNotResponding:
        return "设备无响应";
        
    // 系统错误
    case ErrorType::FileIOError:
        return "文件IO错误";
    case ErrorType::ConfigError:
        return "配置错误";
    case ErrorType::UnknownError:
        return "未知错误";
        
    default:
        return "未知错误";
    }
}

QMessageBox::Icon ErrorHandler::errorTypeToIcon(ErrorType type)
{
    switch (type) {
    // 严重错误 - Critical
    case ErrorType::ConnectionLost:
    case ErrorType::BufferOverflow:
    case ErrorType::FileIOError:
    case ErrorType::UnknownError:
        return QMessageBox::Critical;
        
    // 警告 - Warning
    case ErrorType::ConnectionTimeout:
    case ErrorType::ConnectionFailed:
    case ErrorType::SendFailed:
    case ErrorType::ParseFailed:
    case ErrorType::InvalidData:
    case ErrorType::DataOutOfRange:
    case ErrorType::DeviceLocked:
    case ErrorType::PortLocked:
    case ErrorType::OperationFailed:
    case ErrorType::DeviceNotResponding:
    case ErrorType::ConfigError:
        return QMessageBox::Warning;
        
    // 信息 - Information
    case ErrorType::InvalidFrame:
    case ErrorType::InvalidFrameHeader:
    case ErrorType::InvalidFrameTail:
    case ErrorType::InvalidLength:
        return QMessageBox::Information;
        
    default:
        return QMessageBox::Warning;
    }
}

QString ErrorHandler::getUserFriendlyMessage(ErrorType type)
{
    switch (type) {
    // 网络错误
    case ErrorType::ConnectionTimeout:
        return "连接设备超时，请检查网络连接和设备状态。";
    case ErrorType::ConnectionFailed:
        return "无法连接到设备，请确认设备IP地址和端口号正确。";
    case ErrorType::ConnectionLost:
        return "与设备的连接已断开，请检查网络连接。";
    case ErrorType::SendFailed:
        return "发送数据失败，请检查网络连接。";
        
    // 协议错误
    case ErrorType::InvalidFrame:
        return "接收到无效的数据帧，数据已被丢弃。";
    case ErrorType::InvalidFrameHeader:
        return "数据帧头不正确，数据已被丢弃。";
    case ErrorType::InvalidFrameTail:
        return "数据帧尾不正确，数据已被丢弃。";
    case ErrorType::InvalidLength:
        return "数据长度不正确，数据已被丢弃。";
    case ErrorType::ParseFailed:
        return "解析设备响应失败，请重试。";
        
    // 数据错误
    case ErrorType::InvalidData:
        return "接收到无效的数据，请重试。";
    case ErrorType::DataOutOfRange:
        return "数据超出有效范围，请检查输入。";
    case ErrorType::BufferOverflow:
        return "接收缓冲区溢出，连接将被重置。";
        
    // 设备错误
    case ErrorType::DeviceLocked:
        return "设备已被锁定，请先解锁设备。";
    case ErrorType::PortLocked:
        return "端口已被锁定，请先解锁端口。";
    case ErrorType::OperationFailed:
        return "操作执行失败，请重试。";
    case ErrorType::DeviceNotResponding:
        return "设备无响应，请检查设备状态。";
        
    // 系统错误
    case ErrorType::FileIOError:
        return "文件读写错误，请检查文件权限。";
    case ErrorType::ConfigError:
        return "配置错误，请检查配置文件。";
    case ErrorType::UnknownError:
        return "发生未知错误，请联系技术支持。";
        
    default:
        return "发生错误，请重试。";
    }
}

QString ErrorHandler::getSuggestedAction(ErrorType type)
{
    switch (type) {
    // 网络错误
    case ErrorType::ConnectionTimeout:
        return "1. 检查设备是否开机\n2. 检查网络连接\n3. 确认IP地址正确\n4. 检查防火墙设置";
    case ErrorType::ConnectionFailed:
        return "1. 确认设备IP地址和端口号\n2. 检查设备是否在线\n3. 尝试ping设备IP";
    case ErrorType::ConnectionLost:
        return "系统将自动尝试重新连接，请稍候...";
    case ErrorType::SendFailed:
        return "请检查网络连接后重试";
        
    // 协议错误
    case ErrorType::InvalidFrame:
    case ErrorType::InvalidFrameHeader:
    case ErrorType::InvalidFrameTail:
    case ErrorType::InvalidLength:
        return "如果问题持续出现，请检查设备固件版本";
    case ErrorType::ParseFailed:
        return "请重新读取设备信息";
        
    // 数据错误
    case ErrorType::InvalidData:
        return "请重新读取数据";
    case ErrorType::DataOutOfRange:
        return "请输入有效范围内的数据";
    case ErrorType::BufferOverflow:
        return "连接将自动重置，请稍候...";
        
    // 设备错误
    case ErrorType::DeviceLocked:
        return "请使用解锁功能解锁设备";
    case ErrorType::PortLocked:
        return "请使用解锁功能解锁端口";
    case ErrorType::OperationFailed:
        return "请检查设备状态后重试";
    case ErrorType::DeviceNotResponding:
        return "1. 检查设备是否正常工作\n2. 尝试重启设备\n3. 检查网络连接";
        
    // 系统错误
    case ErrorType::FileIOError:
        return "请检查文件是否存在及权限设置";
    case ErrorType::ConfigError:
        return "请检查配置文件格式是否正确";
    case ErrorType::UnknownError:
        return "请记录错误信息并联系技术支持";
        
    default:
        return "";
    }
}

bool ErrorHandler::isRecoverable(ErrorType type)
{
    switch (type) {
    // 可恢复的错误
    case ErrorType::ConnectionTimeout:
    case ErrorType::ConnectionFailed:
    case ErrorType::BufferOverflow:
    case ErrorType::InvalidFrame:
    case ErrorType::InvalidFrameHeader:
    case ErrorType::InvalidFrameTail:
    case ErrorType::InvalidLength:
        return true;
        
    // 不可恢复的错误
    case ErrorType::ConnectionLost:
    case ErrorType::SendFailed:
    case ErrorType::ParseFailed:
    case ErrorType::InvalidData:
    case ErrorType::DataOutOfRange:
    case ErrorType::DeviceLocked:
    case ErrorType::PortLocked:
    case ErrorType::OperationFailed:
    case ErrorType::DeviceNotResponding:
    case ErrorType::FileIOError:
    case ErrorType::ConfigError:
    case ErrorType::UnknownError:
        return false;
        
    default:
        return false;
    }
}

bool ErrorHandler::requiresImmediateNotification(ErrorType type)
{
    switch (type) {
    // 需要立即通知用户的错误
    case ErrorType::ConnectionLost:
    case ErrorType::SendFailed:
    case ErrorType::ParseFailed:
    case ErrorType::InvalidData:
    case ErrorType::DataOutOfRange:
    case ErrorType::BufferOverflow:
    case ErrorType::DeviceLocked:
    case ErrorType::PortLocked:
    case ErrorType::OperationFailed:
    case ErrorType::DeviceNotResponding:
    case ErrorType::FileIOError:
    case ErrorType::ConfigError:
    case ErrorType::UnknownError:
        return true;
        
    // 不需要立即通知的错误（已自动处理或不严重）
    case ErrorType::ConnectionTimeout:
    case ErrorType::ConnectionFailed:
    case ErrorType::InvalidFrame:
    case ErrorType::InvalidFrameHeader:
    case ErrorType::InvalidFrameTail:
    case ErrorType::InvalidLength:
        return false;
        
    default:
        return true;
    }
}
