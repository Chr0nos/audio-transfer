#ifdef ASIO
#ifndef ASIODEVICE_H
#define ASIODEVICE_H

#include <QObject>
#include <QIODevice>
//#include "./lib/ASIOSDK2.3/common/asio.h"

class AsioDevice : public QIODevice
{
private:
    ASIODriverInfo *info;
    bool start();
    void say(const QString message);
    static ModuleDevice *factory(QString name, AudioFormat *format, void *userData, QObject *parent);
public:
    AsioDevice(QObject *parent);
    bool open(OpenMode mode);
    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);
    int getAvailableChannels();
    void close();
signals:
    void debug(QString message);
};

#endif // ASIODEVICE_H

#endif //ASIO
