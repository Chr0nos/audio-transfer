#ifndef CIRCULARBUFFER_H
#define CIRCULARBUFFER_H

#include <QObject>
#include <QAtomicInt>

class CircularBuffer : public QObject
{
    Q_OBJECT
public:
    explicit CircularBuffer(const uint bufferSize,QObject *parent = 0);
    int getSize();
    bool append(QByteArray newData);
    bool append(const QString text);
    void clear();
    QByteArray getCurrentPosData(int length);
    QByteArray getData();
    int getAvailableBytesCount();
    bool isBufferUnderFeeded();
    static void runTest();
private:
    int bsize;
    QByteArray data;
    int positionRead;
    int positionWrite;

signals:

public slots:

};

#endif // CIRCULARBUFFER_H
