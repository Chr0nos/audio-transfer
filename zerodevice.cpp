#include "zerodevice.h"
#include <QDebug>
#include <QTimer>
//this class is just to be an equivalent to QFile("/dev/zero");

ZeroDevice::ZeroDevice(QObject *parent) :
    QIODevice(parent)
{
    timer = new QTimer(this);
    timer->setInterval(100);
    connect(timer,SIGNAL(timeout()),this,SIGNAL(readyRead()));
}
qint64 ZeroDevice::writeData(const char *data, qint64 len) {
    return len;
}
qint64 ZeroDevice::readData(char *data, qint64 maxlen) {
    memset(data,0,maxlen);
    return maxlen;
}
bool ZeroDevice::open(OpenMode mode) {
    QIODevice::open(mode);
    timer->start();
    if ((mode == QIODevice::ReadOnly) || (mode == QIODevice::ReadWrite)) emit(readyRead());
    return true;
}
