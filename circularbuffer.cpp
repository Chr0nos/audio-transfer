#include "circularbuffer.h"
#include <QDebug>

/* this class provide a circular buffer using a QByteArray inside it: not the most efficiant but should works, currently not used but will be used in the module portaudio
 * */

CircularBuffer::CircularBuffer(const uint bufferSize, QObject *parent) :
    QObject(parent)
{
    bsize = bufferSize;
    data.reserve(bufferSize);
    positionRead = 0;
    positionWrite = 0;
}

quint64 CircularBuffer::getSize() {
    return bsize;
}
bool CircularBuffer::append(QByteArray newData) {
    int lenght = newData.size();
    //Start contains the start position to read for newData
    int start = 0;

    //in case of impossible add: just return false
    if (lenght > bsize) return false;

    //left space betewen current position and the end of buffer
    const int left = bsize - positionWrite -1;

    if (left < lenght) {
        data.insert(positionWrite,newData,left);
        lenght -= left;
        positionWrite = 0;
        start = left;
    }
    data.insert(positionWrite,newData.mid(start,lenght),lenght);
    positionWrite += lenght;
    return true;
}
QByteArray CircularBuffer::getCurrentPosData(int length) {
    //if requested lenght is higher than the buffer himself: returning an empty qbytearray
    if (length > bsize) return QByteArray();

    //left is the size between the end of the buffer and the current position
    const int left = bsize - positionRead;
    QByteArray result;
    if (length > left) {
        length -= left;
        result.append(data.mid(positionRead,left));
        positionRead = 0;
    }
    result.append(data.mid(positionRead,length));
    positionRead += length;
    return result;
}
QByteArray CircularBuffer::getData() {
    //return the whole buffer pointer, this should not be used but who knows... it may be usefull
    return data;
}

void CircularBuffer::clear() {
    data.clear();
    positionRead = 0 ;
    positionWrite = 0;
}
