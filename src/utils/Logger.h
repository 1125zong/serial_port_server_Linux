#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>
#include <QString>
#include <QFile>
#include <QMutex>
#include <QDateTime>

class Logger : public QObject {
    Q_OBJECT
    
public:
    enum Level {    //定义日志级别---默认递增，从上往下
        Debug,      //0
        Info,       //1
        Warning,
        Error,
        Critical
    };
    
    static Logger* instance();  //单例模式---接口函数
    
    void log(Level level, const QString &module, const QString &message);
    void setLogFile(const QString &path);
    void setMaxFileSize(qint64 bytes);
    void enableConsoleOutput(bool enable);
    void setMinLevel(Level level);
    
signals:
    void newLogEntry(const QString &entry);
    
private:
    Logger();   //单例模式---构造函数私有化
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    void writeToFile(const QString &entry);
    void rotateLogFile();
    QString levelToString(Level level);
    
    QFile m_logFile;
    qint64 m_maxFileSize;
    bool m_consoleOutput;
    Level m_minLevel;   //记录最小日志级别
    QMutex m_mutex;     //定义一个互斥锁
    
    static Logger* s_instance;  //单例模式---创建唯一对象
};

#endif // LOGGER_H
