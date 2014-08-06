#include "zerodevice.h"
#include <QDebug>
#include <QTimer>
#include <QTime>
#include "audioformat.h"

//this class is just to be an equivalent to QFile("/dev/zero");

ZeroDevice::ZeroDevice(AudioFormat* format,QObject *parent) :
    QIODevice(parent)
{
    this->format = format;
    bytesCountPs = format->getBitrate();
    lastReadTime = 0;
    timer = new QTimer(this);
    timer->setInterval(100);
    connect(timer,SIGNAL(timeout()),this,SIGNAL(readyRead()));
}
qint64 ZeroDevice::writeData(const char *data, qint64 len) {
    return len;
}
qint64 ZeroDevice::readData(char *data, qint64 maxlen) {
    const int currentTime = QTime::currentTime().msec();
    const int elapsedTime = currentTime - lastReadTime;
    int bytesToRead = format->getBytesSizeForDuration(elapsedTime);
    if (bytesToRead > maxlen) bytesToRead = maxlen;
    else if (bytesToRead < 0) return bytesToRead;
    memset(data,0,bytesToRead);

    lastReadTime = currentTime;
    return bytesToRead;
}
bool ZeroDevice::open(OpenMode mode) {
    QIODevice::open(mode);
    if ((mode == QIODevice::ReadOnly) || (mode == QIODevice::ReadWrite)) {
        emit(readyRead());
        timer->start();
    }
    return true;
}
