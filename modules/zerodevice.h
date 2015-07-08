#ifndef ZERODEVICE_H
#define ZERODEVICE_H

#include <QIODevice>
#include <QTimer>
#include "audioformat.h"
#include "modules/moduledevice.h"

class ZeroDevice : public ModuleDevice
{
    Q_OBJECT
public:
    explicit ZeroDevice(AudioFormat *format, QObject *parent = 0);
    bool open(OpenMode mode);
    qint64 bytesAvailable();
    static ModuleDevice *factory(QString name, AudioFormat *format, void *userData, QObject *parent);

private:
    qint64 writeData(const char *data, qint64 len);
    qint64 readData(char *data, qint64 maxlen);
    QTimer* timer;
    AudioFormat* format;
    quint64 bytesCountPs;
    quint32 lastReadTime;
signals:
    void readyRead();
public slots:

};

#endif // ZERODEVICE_H
