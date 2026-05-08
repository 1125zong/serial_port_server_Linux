#ifndef PORTMODEL_H
#define PORTMODEL_H

#include <QString>

/**
 * @brief Represents a single serial port configuration entry
 * 
 * Configuration file format (tab-separated):
 * [Minor] [ServerIP] [data] [cmd] [FIFO] [SSL] [ttyName] [coutName] [interface] [mode] [BackIP]
 */
class PortModel
{
public:
    PortModel();
    
    // Getters
    int minor() const { return m_minor; }
    QString ip() const { return m_ip; }
    int dataPort() const { return m_dataPort; }
    int cmdPort() const { return m_cmdPort; }
    int fifo() const { return m_fifo; }
    int ssl() const { return m_ssl; }
    QString ttyName() const { return m_ttyName; }
    QString coutName() const { return m_coutName; }
    QString interface() const { return m_interface; }
    int mode() const { return m_mode; }
    QString backupIp() const { return m_backupIp; }
    
    // Setters
    void setMinor(int minor) { m_minor = minor; }
    void setIp(const QString& ip) { m_ip = ip; }
    void setDataPort(int port) { m_dataPort = port; }
    void setCmdPort(int port) { m_cmdPort = port; }
    void setFifo(int fifo) { m_fifo = fifo; }
    void setSsl(int ssl) { m_ssl = ssl; }
    void setTtyName(const QString& name) { m_ttyName = name; }
    void setCoutName(const QString& name) { m_coutName = name; }
    void setInterface(const QString& iface) { m_interface = iface; }
    void setMode(int mode) { m_mode = mode; }
    void setBackupIp(const QString& ip) { m_backupIp = ip; }
    
    // Serialization
    QString toConfigLine() const;
    static PortModel fromConfigLine(const QString& line, bool* ok = nullptr);
    
    // Comparison
    bool operator==(const PortModel& other) const;
    bool operator!=(const PortModel& other) const { return !(*this == other); }
    
private:
    int m_minor;
    QString m_ip;
    int m_dataPort;
    int m_cmdPort;
    int m_fifo;
    int m_ssl;
    QString m_ttyName;
    QString m_coutName;
    QString m_interface;
    int m_mode;
    QString m_backupIp;
};

#endif // PORTMODEL_H
