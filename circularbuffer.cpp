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

int CircularBuffer::getSize() {
    return bsize;
}
bool CircularBuffer::append(QByteArray newData) {
    int lenght = newData.size();
    //Start contains the start position to read for newData
    int start = 0;

    //in case of impossible add: just return false
    if (lenght > bsize) return false;

    //left space betewen current position and the end of buffer
    const int left = bsize - positionWrite;

    if (left < lenght) {
        data.insert(positionWrite,newData,left);
        lenght -= left;
        positionWrite = 0;
        start = left;
    }
    data.replace(positionWrite,lenght,newData.mid(start,lenght));
    positionWrite += lenght;
    return true;
}
bool CircularBuffer::append(const QString text) {
    return this->append(text.toLocal8Bit());
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
bool CircularBuffer::isBufferUnderFeeded() {
    //if the buffer is under feeded: more read than writes, this method will return false, else true
    if (positionRead > positionWrite) return false;
    return true;
}
int CircularBuffer::getAvailableBytesCount() {
    const int lenght = positionWrite - positionRead;
    int available = lenght;
    if (available < 0) {
        available *= -1;
    }

    return available;
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
            if (!quiet) qDebug() << test.getCurrentPosData(bufferLength);

            //adding new letter to buffer
            test.append(QString().fill(letter,patternSize));
        }
    }
    qDebug() << "circular buffer test done";
}
