#ifndef SERVERSOCKET_H
#define SERVERSOCKET_H

#include <QObject>
#include <QTcpServer>
#include <QUdpSocket>
#include "readini.h"

class ServerSocket : public QObject
{
    Q_OBJECT
public:
    explicit ServerSocket(QObject *parent = 0);
    enum type {
        Tcp = 0,
        Udp = 1,
        Invalid = 2
    };
    bool startServer(ServerSocket::type type,int port);
    QUdpSocket* getUdpSocket();
    type getServerType();
    static QString typeToString(ServerSocket::type type);
private:
    QObject *srv;
    type currentType;
    QTextStream* out;
    void say(const QString message);
    QTcpSocket* sockOpenTcp();
    void sockOpenUdp();
    Readini *ini;
private slots:
    void newConnection();
signals:
    void debug(const QString message);
    void sockOpen(QTcpSocket *socket);
    void sockOpen(QUdpSocket *socket);
    void readData(QHostAddress *sender,const QByteArray *data,QUdpSocket* socket);
public slots:

};

#endif // SERVERSOCKET_H
