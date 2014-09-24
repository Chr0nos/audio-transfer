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
    timer->setInterval(10);
    connect(timer,SIGNAL(timeout()),this,SIGNAL(readyRead()));
}
qint64 ZeroDevice::writeData(const char *data, qint64 len) {
    (void) data;
    return len;
}
qint64 ZeroDevice::readData(char *data, qint64 maxlen) {
    const int currentTime = QTime::currentTime().msec();
    const int elapsedTime = currentTime - lastReadTime;
    int bytesToRead = format->getBytesSizeForDuration(elapsedTime);

    lastReadTime = currentTime;
    if (bytesToRead < 0) return -1;
    else if (!elapsedTime) return 0;
    else if (!bytesToRead) return 0;
    else if (bytesToRead > maxlen) bytesToRead = maxlen;
    //qDebug() << "time: " << elapsedTime << " bytes: " << bytesToRead;

    memset(data,0,bytesToRead);

    return bytesToRead;
}
bool ZeroDevice::open(OpenMode mode) {
    QIODevice::open(mode);
    if ((mode == QIODevice::ReadOnly) || (mode == QIODevice::ReadWrite)) {
        lastReadTime = QTime::currentTime().msec();
        emit(readyRead());
        timer->start();
    }
    return true;
}
