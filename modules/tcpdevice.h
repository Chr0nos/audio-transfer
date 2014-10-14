#ifndef TCPDEVICE_H
#define TCPDEVICE_H

#include <QIODevice>
#include <QtNetwork/QTcpSocket>
#include <QString>

#include "audioformat.h"

class TcpDevice : public QIODevice
{
    Q_OBJECT
public:
    explicit TcpDevice(const QString host, const int port,AudioFormat *format,bool sendConfig = true,QObject *parent = 0);
    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);
    bool open(OpenMode mode);
    void close();
private:
    QTcpSocket *sock;
    QString host;
    int port;
    AudioFormat *format;
    void sendFormatSpecs();
    void say(const QString message);
    bool bSendConfig;

signals:
    void debug(const QString message);
public slots:
    void sockClose();
    void sockOpen();
    void stateChanged(QAbstractSocket::SocketState state);
};

#endif // TCPDEVICE_H
