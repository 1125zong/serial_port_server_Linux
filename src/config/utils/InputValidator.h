#ifndef INPUTVALIDATOR_H
#define INPUTVALIDATOR_H

#include <QString>
#include <QPair>

class InputValidator
{
public:
    InputValidator();
    
    // Validation result: <isValid, errorMessage>
    typedef QPair<bool, QString> ValidationResult;
    
    // Validate IP address (IPv4 or IPv6)
    static ValidationResult validateIpAddress(const QString& ip);
    
    // Validate port number (1-65535)
    static ValidationResult validatePortNumber(int port);
    
    // Validate minor number (0-255)
    static ValidationResult validateMinorNumber(int minor);
    
    // Validate TTY name format (e.g., ttyr00, ttyr01)
    static ValidationResult validateTtyName(const QString& ttyName);
    
    // Validate port count (must be 1, 4, 8, or 16)
    static ValidationResult validatePortCount(int count);
    
private:
    static bool isValidIPv4(const QString& ip);
    static bool isValidIPv6(const QString& ip);
};

#endif // INPUTVALIDATOR_H
