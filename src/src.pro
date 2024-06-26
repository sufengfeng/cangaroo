lessThan(QT_MAJOR_VERSION, 5): error("requires Qt 5")

QT += core gui
QT += widgets
QT += xml
QT += charts
QT += serialport
QT += network

TARGET = cangaroo
TEMPLATE = app
CONFIG += warn_on
CONFIG += link_pkgconfig
#CONFIG += console
RC_ICONS = logo.ico
DESTDIR = ../bin
MOC_DIR = ../build/moc
RCC_DIR = ../build/rcc
UI_DIR = ../build/ui
unix:OBJECTS_DIR = ../build/o/unix
win32:OBJECTS_DIR = ../build/o/win32
macx:OBJECTS_DIR = ../build/o/mac


SOURCES += main.cpp\
    mainwindow.cpp \
    mainwindow_terminal.cpp \
    settingsdialog.cpp \
    console.cpp \
    qmywidget.cpp \
    mainwindow_download.cpp \
    workerdownloadthread.cpp
HEADERS  += mainwindow.h \
    mainwindow_terminal.h \
    settingsdialog.h \
    console.h \
    qmywidget.h \
    mainwindow_download.h \
    workerdownloadthread.h

FORMS    += mainwindow.ui \
    mainwindow_terminal.ui \
    settingsdialog.ui \
    mainwindow_terminal.ui \
    settingsdialog.ui \
    mainwindow_download.ui

RESOURCES = cangaroo.qrc

include($$PWD/core/core.pri)
include($$PWD/driver/driver.pri)
include($$PWD/parser/dbc/dbc.pri)
include($$PWD/window/TraceWindow/TraceWindow.pri)
include($$PWD/window/SetupDialog/SetupDialog.pri)
include($$PWD/window/LogWindow/LogWindow.pri)
include($$PWD/window/GraphWindow/GraphWindow.pri)
include($$PWD/window/CanStatusWindow/CanStatusWindow.pri)
include($$PWD/window/RawTxWindow/RawTxWindow.pri)
include($$PWD/buildversion/version.pri)
include($$PWD/QSimpleUpdater/QSimpleUpdater.pri)

unix:PKGCONFIG += libnl-3.0
unix:PKGCONFIG += libnl-route-3.0
unix:include($$PWD/driver/SocketCanDriver/SocketCanDriver.pri)

include($$PWD/driver/CANBlastDriver/CANBlastDriver.pri)
include($$PWD/driver/SLCANDriver/SLCANDriver.pri)


win32:include($$PWD/driver/CandleApiDriver/CandleApiDriver.pri)
DISTFILES += \
    ../build_EXE.bat \
    buildversion/version.txt
!build_pass {
  VERSION = $$butianyun_update_version()
}
