#ifndef CIRCULARDEVICE_H
#define CIRCULARDEVICE_H

#include "circularbuffer.h"
#include "modules/moduledevice.h"

class CircularDevice : public ModuleDevice
{
    Q_OBJECT
public:
    explicit CircularDevice(const uint size, QObject *parent = 0);
    bool open(OpenMode mode);
    qint64 bytesAvailable();
    void clear();
    qint64 write(const QByteArray &data);
private:
    CircularBuffer* buffer;
    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);
    void say(const QString message);
signals:
    void debug(const QString message);
public slots:

};

#endif // CIRCULARDEVICE_H
