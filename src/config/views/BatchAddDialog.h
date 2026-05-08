#ifndef BATCHADDDIALOG_H
#define BATCHADDDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QTextEdit>
#include <QPushButton>
#include <QFormLayout>

class BatchAddDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BatchAddDialog(QWidget *parent = nullptr);
    ~BatchAddDialog();
    
    // Getters for dialog results
    QString getIpAddress() const;
    int getPortCount() const;
    int getStartPort() const;
    bool getSslEnabled() const;
    bool getRedundantMode() const;
    QString getBackupIp() const;

private slots:
    void onIpChanged();
    void onPortCountChanged(int index);
    void onStartPortChanged(int value);
    void onSslChanged(int state);
    void onRedundantChanged(int state);
    void onBackupIpChanged();
    void updatePreview();
    void onAccepted();

private:
    void setupUi();
    void validateInputs();
    
    // UI components
    QLineEdit* m_ipEdit;
    QComboBox* m_portCountCombo;
    QSpinBox* m_startPortSpin;
    QCheckBox* m_sslCheck;
    QCheckBox* m_redundantCheck;
    QLineEdit* m_backupIpEdit;
    QTextEdit* m_previewText;
    QPushButton* m_okButton;
    QPushButton* m_cancelButton;
    
    // Validation state
    bool m_ipValid;
    bool m_backupIpValid;
};

#endif // BATCHADDDIALOG_H
