#include "PortModel.h"
#include <QStringList>
#include <QDebug>

PortModel::PortModel()
    : m_minor(0)
    , m_dataPort(0)
    , m_cmdPort(0)
    , m_fifo(0)
    , m_ssl(0)
    , m_ttyName("ttyw00")
    , m_coutName("cur00")
    , m_interface("0")
    , m_mode(0)
{
}

QString PortModel::toConfigLine() const
{
    QStringList fields;
    fields << QString::number(m_minor);
    fields << m_ip;
    fields << QString::number(m_dataPort);
    fields << QString::number(m_cmdPort);
    fields << QString::number(m_fifo);
    fields << QString::number(m_ssl);
    fields << m_ttyName;
    fields << m_coutName;
    fields << m_interface;
    fields << QString::number(m_mode);
    fields << m_backupIp;
    
    return fields.join("\t");
}

PortModel PortModel::fromConfigLine(const QString& line, bool* ok)
{
    PortModel port;
    
    if (ok) *ok = false;
    
    // 跳过空行和注释
    QString trimmed = line.trimmed();
    if (trimmed.isEmpty() || trimmed.startsWith('#')) {
        return port;
    }
    
    // 按制表符分割
    QStringList fields = line.split('\t', QString::SkipEmptyParts);
    
    // 我们至少需要10个字段（备份IP可以为空）
    if (fields.size() < 10) {
        qWarning() << "Invalid configuration line (insufficient fields):" << line;
        return port;
    }
    
    // 如果需要，填充到11个字段（用于空备份IP）
    while (fields.size() < 11) {
        fields.append(QString());
    }
    
    // 解析字段
    bool convOk = false;
    
    port.m_minor = fields[0].toInt(&convOk);
    if (!convOk) {
        qWarning() << "Invalid minor number:" << fields[0];
        return port;
    }
    
    port.m_ip = fields[1];
    
    port.m_dataPort = fields[2].toInt(&convOk);
    if (!convOk) {
        qWarning() << "Invalid data port:" << fields[2];
        return port;
    }
    
    port.m_cmdPort = fields[3].toInt(&convOk);
    if (!convOk) {
        qWarning() << "Invalid cmd port:" << fields[3];
        return port;
    }
    
    port.m_fifo = fields[4].toInt(&convOk);
    if (!convOk) {
        qWarning() << "Invalid FIFO:" << fields[4];
        return port;
    }
    
    port.m_ssl = fields[5].toInt(&convOk);
    if (!convOk) {
        qWarning() << "Invalid SSL:" << fields[5];
        return port;
    }
    
    port.m_ttyName = fields[6];
    port.m_coutName = fields[7];
    port.m_interface = fields[8];
    
    port.m_mode = fields[9].toInt(&convOk);
    if (!convOk) {
        qWarning() << "Invalid mode:" << fields[9];
        return port;
    }
    
    port.m_backupIp = fields.size() > 10 ? fields[10] : QString();
    
    if (ok) *ok = true;
    return port;
}

bool PortModel::operator==(const PortModel& other) const
{
    return m_minor == other.m_minor &&
           m_ip == other.m_ip &&
           m_dataPort == other.m_dataPort &&
           m_cmdPort == other.m_cmdPort &&
           m_fifo == other.m_fifo &&
           m_ssl == other.m_ssl &&
           m_ttyName == other.m_ttyName &&
           m_coutName == other.m_coutName &&
           m_interface == other.m_interface &&
           m_mode == other.m_mode &&
           m_backupIp == other.m_backupIp;
}
