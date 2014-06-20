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
    explicit TcpDevice(const QString host, const int port,AudioFormat *format,QObject *parent = 0);
    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);
    bool open(OpenMode mode);
private:
    QTcpSocket *sock;
    QString host;
    int port;
    AudioFormat *format;
    void sendFormatSpecs();
    void say(const QString message);

signals:

public slots:
    void sockClose();
    void sockOpen();
};

#endif // TCPDEVICE_H
