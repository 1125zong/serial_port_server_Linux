#ifndef TTYGENERATOR_H
#define TTYGENERATOR_H

#include <QString>

class TtyGenerator
{
public:
    TtyGenerator();
    
    // Generate TTY name from minor number (e.g., 0 -> "ttyr00", 1 -> "ttyr01")
    static QString generateTtyName(int minor);
    
    // Generate cout name from TTY name (e.g., "ttyr00" -> "cur00")
    static QString generateCoutName(const QString& ttyName);
    
    // Extract minor number from TTY name (e.g., "ttyr00" -> 0)
    // Returns -1 if invalid format
    static int getMinorFromTtyName(const QString& ttyName);
    
    // Validate TTY name format
    static bool isValidTtyName(const QString& ttyName);
    
private:
    static const QString TTY_PREFIX;
    static const QString COUT_PREFIX;
};

#endif // TTYGENERATOR_H
