#ifndef SERVERMAIN_H
#define SERVERMAIN_H

#include <QObject>
#include <QtNetwork/QTcpSocket>
#include "server/serversocket.h"
#include "server/userhandler.h"
#include "server/serversecurity.h"
#include "readini.h"
#include "audioformat.h"

class ServerMain : public QObject
{
    Q_OBJECT
public:
    explicit ServerMain(const QString configFilePath,QObject *parent = 0);
    ~ServerMain();
    bool listen(ServerSocket::type type);
    Readini* getIni();
    ServerSecurity* security;
private:
    ServerSocket* srv;
    UserHandler* users;
    Readini *ini;
    AudioFormat *formatDefault;
    QString configFilePath;
signals:
    void debug(const QString message);
public slots:
private slots:
    void say(const QString message);
    void sockOpen(QTcpSocket* newSock);
    void readData(QHostAddress *sender,const quint16 *senderPort,const QByteArray *data,QUdpSocket* udp);
};

#endif // SERVERMAIN_H
