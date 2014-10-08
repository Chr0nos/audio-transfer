#include "circularbuffer.h"
#include <QDebug>
#include <QMutexLocker>

/* this class provide a circular buffer using a QByteArray inside it
 */

CircularBuffer::CircularBuffer(const uint bufferSize, const QString bufferName, QObject *parent) :
    QObject(parent)
{
    bsize = bufferSize;
    originalSize = bsize;
    data.reserve(bufferSize);
    positionRead = 0;
    positionWrite = 0;
    policy = Replace;
    this->setObjectName(bufferName);
    say("buffer initialized");
}
CircularBuffer::~CircularBuffer() {
    say("deleting buffer");
}

int CircularBuffer::getSize() {
    //return the actual buffer size (not the availableBytes sizes)
    return bsize;
}
bool CircularBuffer::append(QByteArray newData) {
    QMutexLocker lock(&mutex);
    int lenght = newData.size();
    const int freeSpace = bsize - getAvailableBytesCount();

    //Start contains the start position to read for newData
    int start = 0;

    //taking care about the case of the lenght to write is higher than the freeSpace (to prevent data drop for example)
    if (freeSpace < lenght) {
        //in normal situation this should NEVER appens but, it the data are not readed fast enoth it will, so use the readyRead signal emited by the class to prevent this
        say("buffer overflow !");
        switch (policy) {
            case Expand: {
                //hum how to handle this... appending data to the buffer and make it size even higher (i'd prefer to dont)
                //warning: use this code could be DANGEREOUS: it can let the class consume all the available memory

                const int missingSpace = lenght - freeSpace;

                //writing possible data to left space
                data.replace(positionWrite,freeSpace,newData.mid(start,freeSpace));

                //here i move the positionRead to the amount of size that i will hadd just few lines below
                if (positionRead > positionWrite) positionRead += missingSpace;

                //decaling the position writePos after the data just replaced
                positionWrite += freeSpace;

                //and inserting the missing left data
                data.insert(positionWrite,newData.mid(freeSpace,missingSpace));

                //ajusting the new buffer size:
                bsize += missingSpace;

                say("expanded buffer size by: " + QString::number(missingSpace));
                emit(readyRead(lenght));
                return true;
            }
            case Drop: {
                //or drop audio
                say("new data has been drop");
                return false;
            }
            case Replace: {
                //relace the older data by new ones
                say("replacing old data by new on (loosing data)");
                //using fallback to do it so no code here but no return^
            }
        }
    }
    //in case of impossible add: just return false
    if (lenght > bsize) {
        say("impossible append detected: data size is higher than buffer size");
        return false;
    }

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
    return true;
}
bool CircularBuffer::append(const char *newData, const int size) {
    return append(QByteArray::fromRawData(newData,size));
}

bool CircularBuffer::append(const QString text) {
    return this->append(text.toLocal8Bit());
}

QByteArray CircularBuffer::getCurrentPosData(int length) {
    QMutexLocker lock(&mutex);

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
QByteArray CircularBuffer::getCurrentPosData() {
    return getCurrentPosData(getAvailableBytesCount());
}

QByteArray CircularBuffer::getData() {
    //return the whole buffer pointer, this should not be used but who knows... it may be usefull
    return data;
}

void CircularBuffer::clear() {
    QMutexLocker lock(&mutex);
    data.clear();
    positionRead = 0 ;
    positionWrite = 0;
}
size_t CircularBuffer::getAvailableBytesCount() {
    const size_t lenght = (bsize + positionWrite - positionRead) % bsize;
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
void CircularBuffer::setOverflowPolicy(const OverflowPolicy newPolicy) {
    policy = newPolicy;
    emit(overflowPolicyChanged(policy));
}
void CircularBuffer::say(const QString message) {
#ifdef DEBUG
    qDebug() << "CircularBuffer: " + this->objectName() + ": " + message;
#endif
    emit(debug(message));
}
