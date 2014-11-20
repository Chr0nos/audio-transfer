#-------------------------------------------------
#
# Project created by QtCreator 2014-02-17T01:16:41
#
#-------------------------------------------------
# Availables defines for this project:
# - MULTIMEDIA : enable native audio output (mandatory)
# - PULSE : enable pulseaudio (unix systems only)
# - PORTAUDIO : enable portaudio support
# - DEBUG : enable the debug mode
# - SERVER : enable the built in server (require COMLINE)
# - COMLINE : enable the command line mode

QT       += core gui network
CONFIG   += c++11
DEFINES += MULTIMEDIA PULSE PORTAUDIO COMLINE SERVER

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = audio-transfer-client
TEMPLATE = app

SOURCES += main.cpp\
        mainwindow.cpp \
    manager.cpp \
    modules/tcpdevice.cpp \
    modules/udpdevice.cpp \
    modules/pulse.cpp \
    modules/zerodevice.cpp \
    modules/nativeaudio.cpp \
    modules/pipedevice.cpp \
    readini.cpp \
    audioformat.cpp \
    size.cpp \
    modules/portaudiodevice.cpp \
    modules/pulsedeviceasync.cpp \
    circularbuffer.cpp \
    graphicgenerator.cpp \
    modules/circulardevice.cpp \
    server/serversecurity.cpp

HEADERS  += mainwindow.h \
    manager.h \
    main.h \
    readini.h \
    modules/pulse.h \
    modules/zerodevice.h \
    modules/tcpdevice.h \
    modules/udpdevice.h \
    modules/nativeaudio.h \
    modules/pipedevice.h \
    audioformat.h \
    size.h \
    modules/portaudiodevice.h \
    modules/pulsedeviceasync.h \
    circularbuffer.h \
    graphicgenerator.h \
    modules/circulardevice.h \
    server/serversecurity.h


FORMS    += mainwindow.ui

contains(DEFINES,COMLINE) {
    CONFIG += console
    SOURCES += comline.cpp
    HEADERS += comline.h

    contains(DEFINES,SERVER) {
	SOURCES += server/serversocket.cpp \
	server/servermain.cpp \
	server/userhandler.cpp \
	server/user.cpp \
	server/afkkiller.cpp

	HEADERS +=    server/serversocket.h \
	server/servermain.h \
	server/userhandler.h \
	server/user.h \
	server/afkkiller.h


	config.files = server/server.ini
	config.path = ${DESTDIR}/etc/audio-transfer/
	INSTALLS += config
    }
}


#comment thoses lines to disable pulseaudio , for windows it disabled because: pulseaudio is a **** bullshit on windows:
#the windows port is version 1.1 and make audio-transfer crash: go thanks the PA developers who absolutly dont care about win32...
win32 {
    contains(DEFINES,PULSE) {
	DEFINES -= PULSE
    }
}
contains(DEFINES,PORTAUDIO) {
    LIBS += -lportaudio
}
contains(DEFINES,PULSE) {
    LIBS += -lpulse-simple -lpulse
 }
contains(DEFINES,MULTIMEDIA) {
    QT += multimedia
}


contains(DEFINES,DEBUG) {
    QMAKE_CFLAGS_DEBUG     += -fsanitize=address -fno-omit-frame-pointer
    QMAKE_CXXFLAGS_DEBUG   += -fsanitize=address -fno-omit-frame-pointer
    QMAKE_LFLAGS_DEBUG     += -fsanitize=address -fno-omit-frame-pointer
}
