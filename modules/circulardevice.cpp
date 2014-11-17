#include "circulardevice.h"
#include <QDebug>

CircularDevice::CircularDevice(const uint size, QObject *parent) :
    QIODevice(parent)
{
    this->buffer = new CircularBuffer(size,"CircularDevice",this);
    connect(buffer,SIGNAL(debug(QString)),this,SIGNAL(debug(QString)));
    //connect(buffer,SIGNAL(readyRead(int)),this,SIGNAL(readyRead()));
    buffer->setOverflowPolicy(CircularBuffer::Drop);
}
bool CircularDevice::open(OpenMode mode) {
    say("opening device");
    if (mode == QIODevice::ReadWrite) {
        QIODevice::open(mode);
        return true;
    }
    return false;
}
qint64 CircularDevice::write(const QByteArray &data) {
    if (data.isEmpty()) return -1;
    if (buffer->append(data)) return data.size();
    return -1;
}

qint64 CircularDevice::writeData(const char *data, qint64 len) {
    if (!buffer->append(QByteArray::fromRawData(data,len))) return -1;
    emit(readyRead());
    return len;
}
qint64 CircularDevice::readData(char *data, qint64 maxlen) {
    //qDebug() << maxlen;
    const int availablesBytes = bytesAvailable();
    if (maxlen < 0) return -1;
    else if (maxlen == 0) return 0;
    else if (!availablesBytes) return 0;

    if (maxlen > availablesBytes) maxlen = availablesBytes;
    //Copy data to new array
    QByteArray raw = buffer->getCurrentPosData(maxlen);
    memcpy(data,raw.data(),raw.size());

    return maxlen;
}
qint64 CircularDevice::bytesAvailable() {
    qint64 available = buffer->getAvailableBytesCount() + QIODevice::bytesAvailable();
    if (available < 0) return 0;
    return available;
}
void CircularDevice::say(const QString message) {
    emit(debug("CircularDevice: " + message));
}
void CircularDevice::clear() {
    buffer->clear();
}
