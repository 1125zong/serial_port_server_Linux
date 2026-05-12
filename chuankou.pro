QT       += core gui network widgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    src/hosting/checkboxcenterdelegate.cpp \
    src/protocol/crc.cpp \
    src/utils/customtitlebar.cpp \
    main.cpp \
    mainwindow.cpp \
    src/controller/DeviceController.cpp \
    src/utils/Utils.cpp \
    src/utils/Logger.cpp \
    src/model/DeviceModel.cpp \
    src/protocol/ProtocolParser.cpp \
    src/protocol/ProtocolBuilder.cpp \
    src/protocol/FrameValidator.cpp \
    src/communication/TcpClient.cpp \
    src/communication/UdpDiscovery.cpp \
    src/communication/ConnectionManager.cpp \
    # Hosting module
    src/hosting/CenterAlignedDelegate.cpp \
    # Configuration module
    src/config/models/PortModel.cpp \
    src/config/models/ConfigDeviceModel.cpp \
    src/config/models/ConfigurationData.cpp \
    src/config/controllers/ConfigManager.cpp \
    src/config/controllers/BackupManager.cpp \
    src/config/controllers/CommandExecutor.cpp \
    src/config/views/ConfigDialog.cpp \
    src/config/views/BatchAddDialog.cpp \
    src/config/views/SelectiveAddDialog.cpp \
    src/config/utils/InputValidator.cpp \
    src/config/utils/PortCalculator.cpp \
    src/config/utils/TtyGenerator.cpp

HEADERS += \
    src/hosting/checkboxcenterdelegate.h \
    src/protocol/FrameValidator.h \
    src/protocol/ProtocolBuilder.h \
    src/protocol/ProtocolParser.h \
    src/protocol/crc.h \
    src/utils/customtitlebar.h \
    src/protocol/data.h \
    mainwindow.h \
    src/controller/DeviceController.h \
    src/utils/ProtocolConstants.h \
    src/utils/Utils.h \
    src/utils/Logger.h \
    src/model/DeviceModel.h \
    src/model/SerialPortConfig.h \
    src/model/NetworkConfig.h \
    src/protocol/ProtocolParser.h \
    src/protocol/ProtocolBuilder.h \
    src/protocol/FrameValidator.h \
    src/communication/TcpClient.h \
    src/communication/UdpDiscovery.h \
    src/communication/ConnectionManager.h \
    # Hosting module
    src/hosting/CenterAlignedDelegate.h \
    # Configuration module
    src/config/models/PortModel.h \
    src/config/models/ConfigDeviceModel.h \
    src/config/models/ConfigurationData.h \
    src/config/controllers/ConfigManager.h \
    src/config/controllers/BackupManager.h \
    src/config/controllers/CommandExecutor.h \
    src/config/views/ConfigDialog.h \
    src/config/views/BatchAddDialog.h \
    src/config/views/SelectiveAddDialog.h \
    src/config/utils/InputValidator.h \
    src/config/utils/PortCalculator.h \
    src/config/utils/TtyGenerator.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    image.qrc \
    qss.qrc

DISTFILES += \
    image/10003.png
