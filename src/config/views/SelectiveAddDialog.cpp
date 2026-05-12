#include "SelectiveAddDialog.h"
#include "../utils/InputValidator.h"
#include "../utils/PortCalculator.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QGridLayout>
#include <QDialogButtonBox>
#include <QScrollArea>

SelectiveAddDialog::SelectiveAddDialog(QWidget *parent)
    : QDialog(parent)
    , m_ipValid(false)
    , m_backupIpValid(true)
{
    setupUi();
}

SelectiveAddDialog::~SelectiveAddDialog()
{
}

void SelectiveAddDialog::setupUi()
{
    setWindowTitle("选择性添加端口");
    resize(700, 600);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // 输入组
    QGroupBox* inputGroup = new QGroupBox("配置参数", this);
    QFormLayout* formLayout = new QFormLayout(inputGroup);
    
    // IP地址
    m_ipEdit = new QLineEdit(this);
    m_ipEdit->setPlaceholderText("例如: 192.168.1.100");
    formLayout->addRow("设备IP地址:", m_ipEdit);
    connect(m_ipEdit, &QLineEdit::textChanged, this, &SelectiveAddDialog::onIpChanged);
    
    // 起始端口
    m_startPortSpin = new QSpinBox(this);
    m_startPortSpin->setRange(950, 950);
    m_startPortSpin->setValue(950);
    m_startPortSpin->setReadOnly(true);
    m_startPortSpin->setButtonSymbols(QAbstractSpinBox::NoButtons);
    formLayout->addRow("起始端口:", m_startPortSpin);
    connect(m_startPortSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &SelectiveAddDialog::onStartPortChanged);
    
    // SSL加密
    m_sslCheck = new QCheckBox("启用SSL加密", this);
    formLayout->addRow("", m_sslCheck);
    connect(m_sslCheck, &QCheckBox::stateChanged, this, &SelectiveAddDialog::onSslChanged);
    
    // 冗余模式
    m_redundantCheck = new QCheckBox("启用冗余模式", this);
    formLayout->addRow("", m_redundantCheck);
    connect(m_redundantCheck, &QCheckBox::stateChanged, this, &SelectiveAddDialog::onRedundantChanged);
    
    // 备份IP
    m_backupIpEdit = new QLineEdit(this);
    m_backupIpEdit->setPlaceholderText("例如: 192.168.1.101");
    m_backupIpEdit->setEnabled(false);
    formLayout->addRow("备份IP地址:", m_backupIpEdit);
    connect(m_backupIpEdit, &QLineEdit::textChanged, this, &SelectiveAddDialog::onBackupIpChanged);
    
    mainLayout->addWidget(inputGroup);
    
    // 端口选择组
    QGroupBox* portGroup = new QGroupBox("端口选择", this);
    QVBoxLayout* portLayout = new QVBoxLayout(portGroup);
    
    // 选择按钮
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    m_selectAllButton = new QPushButton("全选", this);
    m_deselectAllButton = new QPushButton("全不选", this);
    m_selectionCountLabel = new QLabel("已选择: 0 个端口", this);
    
    buttonLayout->addWidget(m_selectAllButton);
    buttonLayout->addWidget(m_deselectAllButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_selectionCountLabel);
    
    connect(m_selectAllButton, &QPushButton::clicked, this, &SelectiveAddDialog::onSelectAllClicked);
    connect(m_deselectAllButton, &QPushButton::clicked, this, &SelectiveAddDialog::onDeselectAllClicked);
    
    portLayout->addLayout(buttonLayout);
    
    // 滚动区域中的端口复选框
    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setMinimumHeight(250);
    
    QWidget* checkboxWidget = new QWidget();
    QGridLayout* gridLayout = new QGridLayout(checkboxWidget);
    
    createPortCheckboxes();
    
    // 将复选框添加到网格中（8列 x 4行）
    for (int i = 0; i < 16; ++i) {
        int row = i / 8;
        int col = i % 8;
        gridLayout->addWidget(m_portCheckboxes[i], row, col);
    }
    
    checkboxWidget->setLayout(gridLayout);
    scrollArea->setWidget(checkboxWidget);
    portLayout->addWidget(scrollArea);
    
    mainLayout->addWidget(portGroup);
    
    // 按钮
    QDialogButtonBox* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_okButton = buttonBox->button(QDialogButtonBox::Ok);
    m_okButton->setText("确定");
    m_okButton->setEnabled(false);
    m_cancelButton = buttonBox->button(QDialogButtonBox::Cancel);
    m_cancelButton->setText("取消");
    
    connect(buttonBox, &QDialogButtonBox::accepted, this, &SelectiveAddDialog::onAccepted);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    
    mainLayout->addWidget(buttonBox);
    
    setLayout(mainLayout);
}

void SelectiveAddDialog::createPortCheckboxes()
{
    for (int i = 1; i <= 16; ++i) {
        QCheckBox* checkbox = new QCheckBox(QString("端口 %1").arg(i), this);
        m_portCheckboxes.append(checkbox);
        connect(checkbox, &QCheckBox::stateChanged, this, &SelectiveAddDialog::onPortCheckboxChanged);
    }
}

QString SelectiveAddDialog::getIpAddress() const
{
    return m_ipEdit->text().trimmed();
}

QList<int> SelectiveAddDialog::getSelectedPortIndices() const
{
    QList<int> indices;
    for (int i = 0; i < m_portCheckboxes.size(); ++i) {
        if (m_portCheckboxes[i]->isChecked()) {
            indices.append(i + 1); // Port indices are 1-based
        }
    }
    return indices;
}

int SelectiveAddDialog::getStartPort() const
{
    return m_startPortSpin->value();
}

bool SelectiveAddDialog::getSslEnabled() const
{
    return m_sslCheck->isChecked();
}

bool SelectiveAddDialog::getRedundantMode() const
{
    return m_redundantCheck->isChecked();
}

QString SelectiveAddDialog::getBackupIp() const
{
    return m_backupIpEdit->text().trimmed();
}

void SelectiveAddDialog::onIpChanged()
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
}

void SelectiveAddDialog::onPortCheckboxChanged()
{
    updateSelectionCount();
    validateInputs();
}

void SelectiveAddDialog::onSelectAllClicked()
{
    for (QCheckBox* checkbox : m_portCheckboxes) {
        checkbox->setChecked(true);
    }
}

void SelectiveAddDialog::onDeselectAllClicked()
{
    for (QCheckBox* checkbox : m_portCheckboxes) {
        checkbox->setChecked(false);
    }
}

void SelectiveAddDialog::onStartPortChanged(int value)
{
    Q_UNUSED(value);
}

void SelectiveAddDialog::onSslChanged(int state)
{
    Q_UNUSED(state);
}

void SelectiveAddDialog::onRedundantChanged(int state)
{
    bool enabled = (state == Qt::Checked);
    m_backupIpEdit->setEnabled(enabled);
    
    if (!enabled) {
        m_backupIpEdit->clear();
        m_backupIpValid = true;
    }
    
    validateInputs();
}

void SelectiveAddDialog::onBackupIpChanged()
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
}

void SelectiveAddDialog::updateSelectionCount()
{
    int count = 0;
    for (QCheckBox* checkbox : m_portCheckboxes) {
        if (checkbox->isChecked()) {
            count++;
        }
    }
    
    m_selectionCountLabel->setText(QString("已选择: %1 个端口").arg(count));
}

void SelectiveAddDialog::validateInputs()
{
    bool valid = m_ipValid && m_backupIpValid;
    
    // At least one port must be selected
    bool hasSelection = false;
    for (QCheckBox* checkbox : m_portCheckboxes) {
        if (checkbox->isChecked()) {
            hasSelection = true;
            break;
        }
    }
    valid = valid && hasSelection;
    
    if (m_redundantCheck->isChecked()) {
        valid = valid && !m_backupIpEdit->text().trimmed().isEmpty();
    }
    
    m_okButton->setEnabled(valid);
}

void SelectiveAddDialog::onAccepted()
{
    // Final validation
    // 检查输入的IP地址是否有效
    if (!m_ipValid) {
        return;
    }
    
    // 检查是否选择了至少一个端口 
    if (getSelectedPortIndices().isEmpty()) {
        return;
    }
    
    // 检查冗余模式下是否输入了有效备份IP地址
    if (m_redundantCheck->isChecked() && !m_backupIpValid) {
        return;
    }
    
    accept();
}
