#include "pipedevice.h"

#include <QDebug>

//this class is herited by QIODevice
//it require "console" in QT arguments

PipeDevice::PipeDevice(const QString name, QObject *parent) :
    QIODevice(parent)
{
    this->setObjectName(name);
    this->hexOutput = false;
    file = NULL;
    buffer = NULL;
    thread = NULL;
}
PipeDevice::~PipeDevice() {
    say("closing device");
    if (file) {
        if (file->isOpen()) file->close();
        delete(file);
    }
    stop();
}
bool PipeDevice::open(OpenMode mode) {
    if (isOpen()) return true;
    say("opening device");
    file = new QFile(this);
    connect(file,SIGNAL(aboutToClose()),this,SLOT(stop()));
    bool state;
    switch (mode) {
        case QIODevice::ReadOnly:
            say("creating buffer 2Mb");
            this->buffer = new CircularBuffer(2097152,this->objectName(),this);
            connect(buffer,SIGNAL(debug(QString)),this,SLOT(say(QString)));
            buffer->setOverflowPolicy(CircularBuffer::Drop);
            buffer->setPacketSize(1024);
            connect(buffer,SIGNAL(readyRead(int)),this,SIGNAL(readyRead()));

            say("creating read class");
            input = new PipeDeviceRead(buffer,1024,0);
            connect(input,SIGNAL(debug(QString)),this,SIGNAL(debug(QString)));

            say("creating new thread");
            thread = new QThread(this);

            say("moving record class to separate thread");
            input->moveToThread(thread);
            connect(thread,SIGNAL(started()),input,SLOT(start()));
            connect(thread,SIGNAL(finished()),this,SLOT(stop()));
            connect(input,SIGNAL(failed()),this,SLOT(stop()));

            say("starting thread");
            thread->start();

            return QIODevice::open(mode);
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
    if (!input) {
        say("read requested but not input available: did you open in ReadOnly mode ?");
        return -1;
    }
    const qint64 available = bytesAvailable();
    if (maxlen > available) maxlen = available;
    if (!maxlen) return 0;

    QByteArray sound = buffer->getCurrentPosData(maxlen);
    //qDebug() << sound.toHex();

    //i copy the buffer into the char*
    memcpy(data,sound.data(),sound.size());
    return sound.size();
}

qint64 PipeDevice::writeData(const char *data, qint64 len) {
    if (!file) return -1;
    if (hexOutput) {
        //this is the hexadecimal output debug
        QByteArray hex = QByteArray::fromRawData(data,len).toHex();
        return file->write(hex);
    }
    return file->write(data,len);
}
void PipeDevice::stop() {
    if (thread) {
        thread->wait(3000);
        thread->terminate();
        thread->disconnect();
        delete(thread);
        thread = NULL;
    }

    if (buffer) {
        buffer->disconnect();
        delete(buffer);
        buffer = NULL;
    }
    this->close();
    say("stoped");
}
void PipeDevice::setHexOutputEnabled(const bool mode) {
    this->hexOutput = mode;
    if (mode) say("switching to hexadecimal debug output");
    else say("disabling hexadecimal mode");
}

qint64 PipeDevice::bytesAvailable() {
    if ((!input) || (!buffer)) return -1;
    return buffer->getAvailableBytesCount() + QIODevice::bytesAvailable();
}


//over this line the class is PipeDeviceRead: it's runing on a separate thread

PipeDeviceRead::PipeDeviceRead(CircularBuffer *buffer, const int blocksize, QObject *parent) {
    this->setParent(parent);
    this->buffer = buffer;
    connect(buffer,SIGNAL(debug(QString)),this,SLOT(say(QString)));
    this->block = blocksize;
    file = new QFile(this);
}

PipeDeviceRead::~PipeDeviceRead() {
    if (file->isOpen()) file->close();
    file->disconnect();
    delete(file);
}

void PipeDeviceRead::start() {
    char *raw;

    say("opening stdin");
    if (!file->open(stdin,QIODevice::ReadOnly)) {
        say("error: unable to open stdin");
        return;
    }

    say("allocating memory");
    raw = new char[block];
    if (!raw)
    {
        say("failed to allocate memory: quiting");
        exit(1);
    }

    say("starting read");
    qint64 bytesRead = 0;
    do {
        //filling small buffer with readed data
        bytesRead = file->read(raw,block);
        //taking care about read error.
        if (bytesRead < 0) say("error: failed to read data from stdin");
    //adding data to the circularBuffer
    } while ((bytesRead >= 0) && (buffer->append(raw,bytesRead)));


    say("error. bytesread: " + QString::number(bytesRead));
    free(raw);
    file->close();
    emit(failed());
}
void PipeDeviceRead::say(const QString message) {
    emit(debug("PipeDeviceRead: " + message));
}
