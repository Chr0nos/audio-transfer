#-------------------------------------------------
#
# Project created by QtCreator 2014-02-17T01:16:41
#
#-------------------------------------------------

QT       += core gui multimedia network
CONFIG   += console c++11
DEFINES += PULSE MULTIMEDIA DEBUG


greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = audio-transfer-client
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    manager.cpp \
    devices.cpp \
    comline.cpp \
    modules/tcpdevice.cpp \
    modules/udpdevice.cpp \
    modules/pulse.cpp \
    modules/zerodevice.cpp \
    modules/nativeaudio.cpp \
    readini.cpp \
    audioformat.cpp \
    tcpsink.cpp

HEADERS  += mainwindow.h \
    manager.h \
    devices.h \
    main.h \
    comline.h \
    readini.h \
    modules/pulse.h \
    modules/zerodevice.h \
    modules/tcpdevice.h \
    modules/nativeaudio.h \
    modules/udpdevices.h \
    audioformat.h \
    tcpsink.h

FORMS    += mainwindow.ui

#comment thoses lines to disable pulseaudio , for windows it disabled because: pulseaudio is a **** bullshit on windows:
#the windows port is version 1.1 and make audio-transfer crash: go thanks the PA developers who absolutly dont care about win32...
!win32 {
    LIBS += -lpulse-simple -lpulse
    DEFINES += PULSE
}
