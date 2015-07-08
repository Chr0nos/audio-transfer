#ifndef TCPDEVICE_H
#define TCPDEVICE_H

#include <QtNetwork/QTcpSocket>
#include <QString>

#include "audioformat.h"
#include "modules/moduledevice.h"

class TcpDevice : public ModuleDevice
{
    Q_OBJECT
public:
    explicit TcpDevice(const QString host, const int port,AudioFormat *format,bool sendConfig = true,QObject *parent = 0);
    bool open(OpenMode mode);
    void close();
    qint64 bytesAvailable();
    static ModuleDevice *factory(QString name, AudioFormat *format, void *userData, QObject *parent);

private:
    QTcpSocket *sock;
    QString host;
    int port;
    AudioFormat *format;
    void sendFormatSpecs();
    void say(const QString message);
    bool bSendConfig;
    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);

signals:
    void debug(const QString message);
public slots:
    void sockClose();
    void sockOpen();
    void stateChanged(QAbstractSocket::SocketState state);
};

#endif // TCPDEVICE_H
