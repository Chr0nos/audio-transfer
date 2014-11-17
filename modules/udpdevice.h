#ifndef UDPDEVICE_H
#define UDPDEVICE_H

#include <QString>
#include <QIODevice>
#include <QtNetwork/QUdpSocket>

#include "audioformat.h"

class UdpDevice : public QIODevice
{
    //Q_OBJECT
public:
    explicit UdpDevice(const QString host,const int port,AudioFormat *format,const bool sendConfig = true,QObject *parent = 0);
    bool open(OpenMode mode);
    void close();
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
    void sockClose();
    void sockOpen();
};

#endif // UDPDEVICE_H
