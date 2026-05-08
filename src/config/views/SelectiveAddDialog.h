#ifndef SELECTIVEADDDIALOG_H
#define SELECTIVEADDDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QList>

class SelectiveAddDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SelectiveAddDialog(QWidget *parent = nullptr);
    ~SelectiveAddDialog();
    
    // Getters for dialog results
    QString getIpAddress() const;
    QList<int> getSelectedPortIndices() const;
    int getStartPort() const;
    bool getSslEnabled() const;
    bool getRedundantMode() const;
    QString getBackupIp() const;

private slots:
    void onIpChanged();
    void onPortCheckboxChanged();
    void onSelectAllClicked();
    void onDeselectAllClicked();
    void onStartPortChanged(int value);
    void onSslChanged(int state);
    void onRedundantChanged(int state);
    void onBackupIpChanged();
    void onAccepted();

private:
    void setupUi();
    void createPortCheckboxes();
    void updateSelectionCount();
    void validateInputs();
    
    // UI components
    QLineEdit* m_ipEdit;
    QSpinBox* m_startPortSpin;
    QCheckBox* m_sslCheck;
    QCheckBox* m_redundantCheck;
    QLineEdit* m_backupIpEdit;
    QLabel* m_selectionCountLabel;
    QPushButton* m_selectAllButton;
    QPushButton* m_deselectAllButton;
    QPushButton* m_okButton;
    QPushButton* m_cancelButton;
    
    // Port checkboxes (1-16)
    QList<QCheckBox*> m_portCheckboxes;
    
    // Validation state
    bool m_ipValid;
    bool m_backupIpValid;
};

#endif // SELECTIVEADDDIALOG_H
