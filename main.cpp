#include "mainwindow.h"

#include <QApplication>
#include <QStyleFactory>
#include <QNetworkProxy>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setStyle(QStyleFactory::create("Windows"));

    // 在程序启动时配置日志系统
    Logger::instance()->setLogFile("application.log");  // 设置日志文件
    Logger::instance()->enableConsoleOutput(true);       // 启用控制台输出
    QNetworkProxy::setApplicationProxy(QNetworkProxy::NoProxy);

    MainWindow w;
    w.show();
    return a.exec();
}
