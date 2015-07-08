#ifndef UDPDEVICE_H
#define UDPDEVICE_H

#include <QString>
#include <QtNetwork/QUdpSocket>

#include "audioformat.h"
#include "modules/moduledevice.h"

class UdpDevice : public ModuleDevice
{
    Q_OBJECT
public:
    explicit UdpDevice(const QString host,const int port,AudioFormat *format,const bool sendConfig = true,QObject *parent = 0);
    bool open(OpenMode mode);
    void close();
    static ModuleDevice *factory(QString name, AudioFormat *format, void *userData, QObject *parent);
private:
    QUdpSocket *sock;
    QString host;
    int port;
    void say(const QString message);
    AudioFormat *format;
    bool bSendConfig;
    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);

signals:
    void debug(const QString message);
public slots:

};

#endif // UDPDEVICE_H
