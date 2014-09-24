#ifndef CIRCULARBUFFER_H
#define CIRCULARBUFFER_H

#include <QObject>
#include <QMutex>

class CircularBuffer : public QObject
{
    Q_OBJECT
public:
    explicit CircularBuffer(const uint bufferSize,const QString bufferName = "",QObject *parent = 0);
    int getSize();
    bool append(QByteArray newData);
    bool append(const QString text);
    bool append(const char* newData,const int size);
    void clear();
    QByteArray getCurrentPosData(int length);
    QByteArray getCurrentPosData();
    QByteArray getData();
    size_t getAvailableBytesCount();
    static void runTest();
    void operator <<(QByteArray newData);
    QByteArray operator >>(const int lenght);
    enum OverflowPolicy {
        Drop = 0,
        Expand = 1
    };
    void setOverflowPolicy(const OverflowPolicy newPolicy);

private:
    int bsize;
    QByteArray data;
    int positionRead;
    int positionWrite;
    QMutex mutex;
    OverflowPolicy policy;
    void say(const QString message);

signals:
    void readyRead(const int size);
    void overflowPolicyChanged(const OverflowPolicy newPolicy);
    void debug(const QString message);
public slots:

};

#endif // CIRCULARBUFFER_H
