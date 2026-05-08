#include "BatchAddDialog.h"
#include "../utils/InputValidator.h"
#include "../utils/PortCalculator.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QDialogButtonBox>

BatchAddDialog::BatchAddDialog(QWidget *parent)
    : QDialog(parent)
    , m_ipValid(false)
    , m_backupIpValid(true)
{
    setupUi();
    updatePreview();
}

BatchAddDialog::~BatchAddDialog()
{
}

void BatchAddDialog::setupUi()
{
    setWindowTitle("批量添加端口");
    resize(600, 500);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // 输入组
    QGroupBox* inputGroup = new QGroupBox("配置参数", this);
    QFormLayout* formLayout = new QFormLayout(inputGroup);
    
    // IP地址
    m_ipEdit = new QLineEdit(this);
    m_ipEdit->setPlaceholderText("例如: 192.168.1.100");
    formLayout->addRow("设备IP地址:", m_ipEdit);
    connect(m_ipEdit, &QLineEdit::textChanged, this, &BatchAddDialog::onIpChanged);
    
    // 端口数量
    m_portCountCombo = new QComboBox(this);
    m_portCountCombo->addItem("1 端口", 1);
    m_portCountCombo->addItem("4 端口", 4);
    m_portCountCombo->addItem("8 端口", 8);
    m_portCountCombo->addItem("16 端口", 16);

    m_portCountCombo->setCurrentIndex(0);
    formLayout->addRow("端口数量:", m_portCountCombo);
    connect(m_portCountCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &BatchAddDialog::onPortCountChanged);
    
    // 起始端口
    m_startPortSpin = new QSpinBox(this);
    m_startPortSpin->setRange(1, 65535);
    m_startPortSpin->setValue(950);
    formLayout->addRow("起始端口:", m_startPortSpin);
    connect(m_startPortSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &BatchAddDialog::onStartPortChanged);
    
    // SSL加密
    m_sslCheck = new QCheckBox("启用SSL加密", this);
    formLayout->addRow("", m_sslCheck);
    connect(m_sslCheck, &QCheckBox::stateChanged, this, &BatchAddDialog::onSslChanged);
    
    // 冗余模式
    m_redundantCheck = new QCheckBox("启用冗余模式", this);
    formLayout->addRow("", m_redundantCheck);
    connect(m_redundantCheck, &QCheckBox::stateChanged, this, &BatchAddDialog::onRedundantChanged);
    
    // 备份IP
    m_backupIpEdit = new QLineEdit(this);
    m_backupIpEdit->setPlaceholderText("例如: 192.168.1.101");
    m_backupIpEdit->setEnabled(false);
    formLayout->addRow("备份IP地址:", m_backupIpEdit);
    connect(m_backupIpEdit, &QLineEdit::textChanged, this, &BatchAddDialog::onBackupIpChanged);
    
    mainLayout->addWidget(inputGroup);
    
    // 预览组
    QGroupBox* previewGroup = new QGroupBox("预览", this);
    QVBoxLayout* previewLayout = new QVBoxLayout(previewGroup);
    
    m_previewText = new QTextEdit(this);
    m_previewText->setReadOnly(true);
    m_previewText->setMaximumHeight(150);
    previewLayout->addWidget(m_previewText);
    
    mainLayout->addWidget(previewGroup);
    
    // 按钮
    QDialogButtonBox* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_okButton = buttonBox->button(QDialogButtonBox::Ok);
    m_okButton->setText("确定");
    m_okButton->setEnabled(false);
    m_cancelButton = buttonBox->button(QDialogButtonBox::Cancel);
    m_cancelButton->setText("取消");
    
    connect(buttonBox, &QDialogButtonBox::accepted, this, &BatchAddDialog::onAccepted);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    
    mainLayout->addWidget(buttonBox);
    
    setLayout(mainLayout);
}

QString BatchAddDialog::getIpAddress() const
{
    return m_ipEdit->text().trimmed();
}

int BatchAddDialog::getPortCount() const
{
    return m_portCountCombo->currentData().toInt();
}

int BatchAddDialog::getStartPort() const
{
    return m_startPortSpin->value();
}

bool BatchAddDialog::getSslEnabled() const
{
    return m_sslCheck->isChecked();
}

bool BatchAddDialog::getRedundantMode() const
{
    return m_redundantCheck->isChecked();
}

QString BatchAddDialog::getBackupIp() const
{
    return m_backupIpEdit->text().trimmed();
}

void BatchAddDialog::onIpChanged()
{
    QString ip = m_ipEdit->text().trimmed();
    auto result = InputValidator::validateIpAddress(ip);
    m_ipValid = result.first;
    
    if (!ip.isEmpty() && !m_ipValid) {
        m_ipEdit->setStyleSheet("QLineEdit { background-color: #ffcccc; }");
    } else {
        m_ipEdit->setStyleSheet("");
    }
    
    validateInputs();
    updatePreview();
}

void BatchAddDialog::onPortCountChanged(int index)
{
    Q_UNUSED(index);
    updatePreview();
}

void BatchAddDialog::onStartPortChanged(int value)
{
    Q_UNUSED(value);
    updatePreview();
}

void BatchAddDialog::onSslChanged(int state)
{
    Q_UNUSED(state);
    updatePreview();
}

void BatchAddDialog::onRedundantChanged(int state)
{
    bool enabled = (state == Qt::Checked);
    m_backupIpEdit->setEnabled(enabled);
    
    if (!enabled) {
        m_backupIpEdit->clear();
        m_backupIpValid = true;
    }
    
    validateInputs();
    updatePreview();
}

void BatchAddDialog::onBackupIpChanged()
{
    QString backupIp = m_backupIpEdit->text().trimmed();
    
    if (m_redundantCheck->isChecked()) {
        if (backupIp.isEmpty()) {
            m_backupIpValid = false;
        } else {
            auto result = InputValidator::validateIpAddress(backupIp);
            m_backupIpValid = result.first;
        }
        
        if (!backupIp.isEmpty() && !m_backupIpValid) {
            m_backupIpEdit->setStyleSheet("QLineEdit { background-color: #ffcccc; }");
        } else {
            m_backupIpEdit->setStyleSheet("");
        }
    } else {
        m_backupIpValid = true;
    }
    
    validateInputs();
    updatePreview();
}

void BatchAddDialog::updatePreview()
{
    QString preview;
    
    if (!m_ipValid) {
        preview = "请输入有效的IP地址";
        m_previewText->setPlainText(preview);
        return;
    }
    
    QString ip = getIpAddress();
    int portCount = getPortCount();
    int startPort = getStartPort();
    bool ssl = getSslEnabled();
    bool redundant = getRedundantMode();
    QString backupIp = getBackupIp();
    
    preview += QString("设备IP: %1\n").arg(ip);
    preview += QString("端口数量: %1\n").arg(portCount);
    preview += QString("SSL: %1\n").arg(ssl ? "是" : "否");
    preview += QString("冗余模式: %1\n").arg(redundant ? "是" : "否");
    if (redundant && !backupIp.isEmpty()) {
        preview += QString("备份IP: %1\n").arg(backupIp);
    }
    preview += "\n将添加以下端口:\n";
    
    for (int i = 1; i <= portCount; ++i) {
        QPair<int, int> ports = PortCalculator::calculatePorts(i, startPort);
        preview += QString("端口 %1: 数据端口=%2, 命令端口=%3\n")
                   .arg(i).arg(ports.first).arg(ports.second);
    }
    
    m_previewText->setPlainText(preview);
}

void BatchAddDialog::validateInputs()
{
    bool valid = m_ipValid && m_backupIpValid;
    
    if (m_redundantCheck->isChecked()) {
        valid = valid && !m_backupIpEdit->text().trimmed().isEmpty();
    }
    
    m_okButton->setEnabled(valid);
}

void BatchAddDialog::onAccepted()
{
    // Final validation
    if (!m_ipValid) {
        return;
    }
    
    if (m_redundantCheck->isChecked() && !m_backupIpValid) {
        return;
    }
    
    accept();
}
