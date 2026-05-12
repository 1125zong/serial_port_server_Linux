#include "mainwindow.h"

#include <QApplication>
#include <QStyleFactory>
#include <QNetworkProxy>
#include <QFile>
#include <QTextStream>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setStyle(QStyleFactory::create("Windows"));
    QFile file(":/qss/Style_1.qss");
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream filetext(&file);
        QString styleSheet = filetext.readAll();
        a.setStyleSheet(styleSheet);
        file.close();
    } else {
        qDebug() << "open qss failed:" << file.errorString();
    }

    // 在程序启动时配置日志系统
    Logger::instance()->setLogFile("application.log");  // 设置日志文件
    Logger::instance()->enableConsoleOutput(true);       // 启用控制台输出
    QNetworkProxy::setApplicationProxy(QNetworkProxy::NoProxy);

    MainWindow w;
    w.show();
    return a.exec();
}
