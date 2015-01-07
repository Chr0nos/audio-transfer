#include "zerodevice.h"
#include <QDebug>
#include <QTimer>
#include <QTime>
#include "audioformat.h"

//this class is just to an equivalent to QFile("/dev/zero");
//but cross platform and with the good bitrate

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
    (void) data;
    return len;
}
qint64 ZeroDevice::readData(char *data, qint64 maxlen) {
    int bytesToRead = bytesAvailable();

    if (maxlen < 0) return -1;
    else if (!bytesToRead) return 0;
    else if (bytesToRead < maxlen) maxlen = bytesToRead;
    //qDebug() << "time: " << elapsedTime << " bytes: " << bytesToRead;

    memset(data,0,maxlen);

    return maxlen;
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
qint64 ZeroDevice::bytesAvailable() {
    const int currentTime = QTime::currentTime().msec();
    const int elapsedTime = currentTime - lastReadTime;
    if (!elapsedTime) return 0;
    return format->getBytesSizeForDuration(elapsedTime) + QIODevice::bytesAvailable();
}
