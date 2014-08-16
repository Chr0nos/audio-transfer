#ifndef CIRCULARBUFFER_H
#define CIRCULARBUFFER_H

#include <QObject>
#include <QAtomicInt>

class CircularBuffer : public QObject
{
    Q_OBJECT
public:
    explicit CircularBuffer(const uint bufferSize,QObject *parent = 0);
    quint64 getSize();
    bool append(QByteArray newData);
    void clear();
    QByteArray getCurrentPosData(int lenght);
    QByteArray getData();
private:
    int bsize;
    QByteArray data;
    QAtomicInt positionRead;
    QAtomicInt positionWrite;

signals:

public slots:

};

#endif // CIRCULARBUFFER_H
