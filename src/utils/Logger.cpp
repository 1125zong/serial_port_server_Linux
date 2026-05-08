#include "Logger.h"
#include <QTextStream>
#include <QDir>
#include <QDebug>



Logger::Logger()
    //构造函数初始化列表
    : m_maxFileSize(10 * 1024 * 1024)  // 10MB --- 日志最大容量
    , m_consoleOutput(false)
    , m_minLevel(Info)      // 最小日志级别为Info
{
}

Logger::~Logger() {
    if (m_logFile.isOpen()) {
        m_logFile.close();
    }
}

Logger* Logger::instance() {    //接口函数
static Logger instance; // 线程安全，自动销毁
return &instance;
}

void Logger::log(Level level, const QString &module, const QString &message) {
    if (level < m_minLevel) {   //如果日志级别小于Info，则不会被记录
        return;
    }
    
    QMutexLocker locker(&m_mutex);      //管理互斥锁m_mutex，构造时上锁，析构时自动解锁
    
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
    QString levelStr = levelToString(level);    //编译后通常只保留整数值，因此需要借用levelToString函数将level转为对应的字符串
    QString entry = QString("[%1] [%2] [%3] %4")
                        .arg(timestamp)
                        .arg(levelStr)
                        .arg(module)
                        .arg(message);
    
    if (m_consoleOutput)    //为false时，就输出entry
    {
        qDebug().noquote() << entry;
    }
    
    if (m_logFile.isOpen())     //判断文件是否打开，如果打开将entry写入文件m_logFile中
    {
        writeToFile(entry);
    }
    
    emit newLogEntry(entry);    //发送一个自定义信号
}

void Logger::setLogFile(const QString &path) {
    QMutexLocker locker(&m_mutex);
    
    if (m_logFile.isOpen()) {       //如果文件打开，就关闭文件
        m_logFile.close();
    }
    
    // 确保目录存在，如果不存在就在当前目录下创建该目录
    QFileInfo fileInfo(path);
    QDir dir = fileInfo.dir();
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    m_logFile.setFileName(path);        //设置文件m_logFile的路径为path
    if (!m_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        qWarning() << "无法打开日志文件:" << path;
    }
}

void Logger::setMaxFileSize(qint64 bytes) {
    m_maxFileSize = bytes;
}

void Logger::enableConsoleOutput(bool enable) {
    m_consoleOutput = enable;
}

void Logger::setMinLevel(Level level) {
    m_minLevel = level;
}

void Logger::writeToFile(const QString &entry) {
    if (!m_logFile.isOpen()) {
        return;
    }
    
    // 检查文件大小
    if (m_logFile.size() > m_maxFileSize) {
        rotateLogFile();
    }
    
    QTextStream stream(&m_logFile);
    stream << entry << "\n";
    m_logFile.flush();
}

void Logger::rotateLogFile() {
    if (!m_logFile.isOpen()) {
        return;
    }
    
    QString currentPath = m_logFile.fileName();
    m_logFile.close();
    
    // 生成归档文件名
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    QString archivePath = currentPath + "." + timestamp;
    
    // 重命名当前文件
    QFile::rename(currentPath, archivePath);
    
    // 重新打开日志文件
    m_logFile.setFileName(currentPath);
    if (!m_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        qWarning() << "无法重新打开日志文件:" << currentPath;
    }
}

QString Logger::levelToString(Level level) {
    switch (level) {
        case Debug:    return "DEBUG";
        case Info:     return "INFO";
        case Warning:  return "WARN";
        case Error:    return "ERROR";
        case Critical: return "CRITICAL";
        default:       return "UNKNOWN";
    }
}
