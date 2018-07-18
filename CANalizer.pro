#-------------------------------------------------
#
# Project created by QtCreator 2018-07-18T10:01:40
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = CANalizer
TEMPLATE = app

CONFIG += c++14

SOURCES += main.cpp\
        mainwindow.cpp \
    logmodel.cpp

HEADERS  += mainwindow.h \
    logmodel.h

FORMS    += mainwindow.ui
