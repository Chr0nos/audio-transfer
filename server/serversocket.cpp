#include "serversocket.h"
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QUdpSocket>
#include "server/security/serversecurity.h"
#include "server/servermain.h"

ServerSocket::ServerSocket(QObject *parent) :
    QObject(parent)
{
}

QTcpServer* ServerSocket::makeTcpServer(const int port)
{
    QTcpServer *serv;

    serv = new QTcpServer(this);
    connect(serv,SIGNAL(newConnection()), this, SLOT(newConnection()));
    if (!serv->listen(QHostAddress::Any, port))
    {
        say("cannot bind port: " + QString::number(port));
        serv->deleteLater();
        exit(0);
        return 0;
    }
    return serv;
}

QUdpSocket* ServerSocket::makeUdpServer(const int port)
{
    QUdpSocket *sock;

    sock = new QUdpSocket(this);
    if (!sock->bind(port))
    {
        say("cannot bind port: " + QString::number(port));
        sock->deleteLater();
        exit(0);
        return 0;
    }
    connect(sock, SIGNAL(readyRead()), this, SLOT(newConnection()));
    return sock;
}

bool ServerSocket::startServer(ServerSocket::type type, const int port)
{
    this->srv = NULL;
    this->currentType = type;

    if (type == ServerSocket::Tcp) this->srv = (QObject*) makeTcpServer(port);
    else if (type == ServerSocket::Udp) this->srv = (QObject*) makeUdpServer(port);
    return (bool) this->srv;
}

void ServerSocket::newConnection()
{
    QTcpSocket* sock;

    if (!this->srv) return;
    if (this->currentType == Tcp)
    {
        say("sockopen.");
        sock = ((QTcpServer*)(this->srv))->nextPendingConnection();
        if (this->newConnectionCheckTcp(sock))
        {
            emit(sockOpen(sock));
        }
    }
    else sockOpenUdp();
}

bool ServerSocket::newConnectionCheckTcp(QTcpSocket *sock)
{
    ServerSecurity* security;
    QHostAddress peer;

    security = qobject_cast<ServerMain*>(this->parent())->security;
    peer = sock->peerAddress();
    if (!security->isAuthorisedHost(&peer))
    {
        const QString host = sock->peerAddress().toString();
        sock->close();
        sock->deleteLater();
        say("refused tcp incoming connection from " + host);
        return false;
    }
    return true;
}

void ServerSocket::say(const QString message)
{
    emit(debug(message));
}

void ServerSocket::sockOpenUdp()
{
    /*
    ** this method is actualy called on every packets received
    ** in udp mode
    */
    QUdpSocket *udp;
    QByteArray datagram;
    QHostAddress sender;
    quint16 senderPort;
    quint64 size;

    udp = (QUdpSocket*) this->srv;
    while (udp->hasPendingDatagrams())
    {
        size = udp->pendingDatagramSize();
        datagram.clear();
        if (size > (quint64) datagram.size())
        {
            datagram.resize(size);
        }
        udp->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
        emit(readData(&sender, &datagram));
    }
}

QUdpSocket* ServerSocket::getUdpSocket()
{
    if (currentType == Tcp) return 0;
    return (QUdpSocket*) this->srv;
}

ServerSocket::type ServerSocket::getServerType()
{
    return currentType;
}

QString ServerSocket::typeToString(ServerSocket::type type)
{
    switch (type)
    {
        case Udp:
            return QString("Udp");
        case Tcp:
            return QString("Tcp");
        case Invalid:
            return QString("Invalid");
    }
    return QString("Unknow");
}
QObject* ServerSocket::getSocketPointer()
{
    return this->srv;
}

