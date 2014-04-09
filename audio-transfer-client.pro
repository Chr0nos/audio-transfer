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
    manager.cpp \
    devices.cpp \
    comline.cpp \
    tcpsink.cpp \
    pulse.cpp

HEADERS  += mainwindow.h \
    manager.h \
    devices.h \
    main.h \
    comline.h \
    tcpsink.h \
    pulse.h

FORMS    += mainwindow.ui

#comment thoses lines to disable pulseaudio
win32:LIBS += c:\pulseaudio\bin\libpulse-simple-0.dll c:\pulseaudio\bin\libpulse-0.dll
else:LIBS += -lpulse-simple -lpulse
DEFINES += PULSE
