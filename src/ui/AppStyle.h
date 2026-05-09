#ifndef APPSTYLE_H
#define APPSTYLE_H

#include <QString>

class QApplication;
class QDialog;
class QWidget;

class AppStyle
{
public:
    static QString styleSheet();
    static void apply(QApplication *app);
    static void apply(QWidget *widget, bool appendCurrentStyle = true);
    static void prepareMainWindow(QWidget *window);
    static void prepareDialog(QDialog *dialog);
    static void refresh(QWidget *widget);
    static void setInvalid(QWidget *widget, bool invalid);
    static void setMuted(QWidget *widget, bool muted);

private:
    static QString readResource(const QString &path);
    static void setState(QWidget *widget, const char *name, bool enabled);
};

#endif // APPSTYLE_H
