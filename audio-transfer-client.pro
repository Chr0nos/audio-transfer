#-------------------------------------------------
#
# Project created by QtCreator 2014-02-17T01:16:41
#
#-------------------------------------------------

QT       += core gui multimedia network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = audio-transfer-client
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
        qrec.cpp \
    manager.cpp \
    devices.cpp

HEADERS  += mainwindow.h \
            qrec.h \
    manager.h \
    devices.h

FORMS    += mainwindow.ui
