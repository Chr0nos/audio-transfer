#ifndef CIRCULARBUFFER_H
#define CIRCULARBUFFER_H

#include <QObject>
#include <QMutex>

class CircularBuffer : public QObject
{
    Q_OBJECT
public:
    explicit CircularBuffer(const uint bufferSize,QObject *parent = 0);
    int getSize();
    bool append(QByteArray newData);
    bool append(const QString text);
    bool append(const char* newData,const int size);
    void clear();
    QByteArray getCurrentPosData(int length);
    QByteArray getCurrentPosData();
    QByteArray getData();
    int getAvailableBytesCount();
    bool isBufferUnderFeeded();
    static void runTest();
    void operator <<(QByteArray newData);
    QByteArray operator >>(const int lenght);
private:
    int bsize;
    QByteArray data;
    int positionRead;
    int positionWrite;
    QMutex mutex;

signals:
    void readyRead(const int size);
public slots:

};

#endif // CIRCULARBUFFER_H
