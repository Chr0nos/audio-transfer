#include "pipedevice.h"
#include <QDebug>
PipeDevice::PipeDevice(const QString name, QObject *parent) :
    QIODevice(parent)
{
    this->setObjectName(name);
    file = NULL;
    timer = new QTimer(this);
    timer->setInterval(100);
    connect(timer,SIGNAL(timeout()),this,SIGNAL(readyRead()));
}
PipeDevice::~PipeDevice() {
    say("closing device");
    if (file) {
        if (file->isOpen()) file->close();
        delete(file);
    }
    delete(timer);
}
bool PipeDevice::open(OpenMode mode) {
    say("opening device");
    file = new QFile(this);
    connect(file,SIGNAL(aboutToClose()),this,SLOT(stop()));
    bool state;
    switch (mode) {
        case QIODevice::ReadOnly:
            connect(file,SIGNAL(readyRead()),this,SIGNAL(readyRead()));
            state = file->open(stdin,mode);
            if (state) {
                say("opened stdin successfully");
                timer->start();
                QIODevice::open(mode);
                return state;
            }
            break;
        case QIODevice::WriteOnly:
            state = file->open(stdout,mode);
            if (state) {
                say("opened stdout successfully");
                QIODevice::open(mode);
                return state;
            }
            break;
        case QIODevice::ReadWrite:
            say("readWrite: unsuported mode");
            delete(file);
            file = NULL;
            return false;
    }

    return false;
}
void PipeDevice::say(const QString message) {
    emit(debug("PipeDevice: " + message));
}
qint64 PipeDevice::readData(char *data, qint64 maxlen) {
    if (!file) return -1;
    return file->read(data,maxlen);
}

qint64 PipeDevice::writeData(const char *data, qint64 len) {
    if (!file) return -1;
    return file->write(data,len);
}
void PipeDevice::stop() {
    say("stoped");
}
