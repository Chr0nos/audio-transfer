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
    mutex = NULL;
}
void CircularBuffer::setMutexEnabled(const bool enableMutex) {
    if (enableMutex) {
        if (mutex) return;
        mutex = new QMutex;
    }
    else if (mutex) {
        delete(mutex);
        mutex = NULL;
    }
}

int CircularBuffer::getSize() {
    return bsize;
}
bool CircularBuffer::append(QByteArray newData) {
    int lenght = newData.size();
    //Start contains the start position to read for newData
    int start = 0;

    //in case of impossible add: just return false
    if (lenght > bsize) return false;

    //doing the mutex lock
    if (mutex) mutex->lock();

    //left space betewen current position and the end of buffer
    const int left = bsize - positionWrite;

    if (left < lenght) {
        data.replace(positionWrite,left,newData.mid(start,left));
        lenght -= left;
        positionWrite = 0;
        start = left;
    }
    //replace the old data with new ones
    data.replace(positionWrite,lenght,newData.mid(start,lenght));
    //Appending lenght to the current write position
    positionWrite += lenght;
    emit(readyRead(lenght));
    if (mutex) mutex->unlock();
    return true;
}
bool CircularBuffer::append(const char *newData, const int size) {
    return append(QByteArray::fromRawData(newData,size));
}

bool CircularBuffer::append(const QString text) {
    return this->append(text.toLocal8Bit());
}

QByteArray CircularBuffer::getCurrentPosData(int length) {
    //if requested lenght is higher than the buffer himself: returning an empty qbytearray
    if (length > bsize) return QByteArray();

    if (mutex) mutex->lock();
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
    if (mutex) mutex->unlock();
    return result;
}
QByteArray CircularBuffer::getCurrentPosData() {
    return getCurrentPosData(getAvailableBytesCount());
}

QByteArray CircularBuffer::getData() {
    //return the whole buffer pointer, this should not be used but who knows... it may be usefull
    return data;
}

void CircularBuffer::clear() {
    mutex->lock();
    data.clear();
    positionRead = 0 ;
    positionWrite = 0;
    mutex->unlock();
}
bool CircularBuffer::isBufferUnderFeeded() {
    //if the buffer is under feeded: more read than writes, this method will return false, else true
    if (positionRead > positionWrite) return false;
    return true;
}
int CircularBuffer::getAvailableBytesCount() {
    const int lenght = (bsize + positionWrite - positionRead) % bsize;
    return lenght;
}
void CircularBuffer::runTest() {
    const int patternSize = 2;
    const int bufferLength = 16;
    int tests = 1;
    const bool quiet = false;

    //this method run a small test for the class and make sure it's still working

    qDebug() << "circular buffer test start";
    //create the buffer
    CircularBuffer test(bufferLength);

    //filling it with "." for all the buffer length
    test.data.fill(46,bufferLength);

    //lets do the amounth of tests requied
    while (tests--) {
        //putting every lettrer from A to Z in the buffer
        for (int i = 65 ; i < 92 ; i++) {
            //making a QChar from the current i value
            char letter = (char) i;

            //showing current buffer content if quiet mode is not disabled
            if (!quiet) qDebug() << test.getCurrentPosData(bufferLength) << test.getAvailableBytesCount();

            //adding new letter to buffer
            test.append(QString().fill(letter,patternSize));
        }
    }
    qDebug() << "circular buffer test done";
}
void CircularBuffer::operator <<(QByteArray newData) {
    append(newData);
}
QByteArray CircularBuffer::operator >>(const int lenght) {
    return getCurrentPosData(lenght);
}
