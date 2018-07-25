#-------------------------------------------------
#
# Project created by QtCreator 2018-07-18T10:01:40
#
#-------------------------------------------------

lessThan(QT_MAJOR_VERSION, 5) {
    message("Cannot build current CANalizer sources with Qt version $${QT_VERSION}.")
}

QT       += core gui widgets serialbus

TARGET = CANalizer
TEMPLATE = app

CONFIG += c++14

SOURCES += main.cpp\
        mainwindow.cpp \
    logmodel.cpp \
    logdialog.cpp \
    capturedialog.cpp

HEADERS  += mainwindow.h \
    logmodel.h \
    logdialog.h \
    capturedialog.h

FORMS    += mainwindow.ui \
    logdialog.ui \
    capturedialog.ui
