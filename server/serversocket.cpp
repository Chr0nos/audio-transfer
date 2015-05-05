#include "serversocket.h"
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QUdpSocket>
#include "server/security/serversecurity.h"
#include "server/servermain.h"

ServerSocket::ServerSocket(QObject *parent) :
    QObject(parent)
{
}
bool ServerSocket::startServer(ServerSocket::type type, const int port)
{
    QTcpServer *serv;
    QUdpSocket *sock;

    this->currentType = type;
    switch (type) {
        case ServerSocket::Tcp: {
            serv = new QTcpServer(this);
            connect(serv,SIGNAL(newConnection()),this,SLOT(newConnection()));
            if (!serv->listen(QHostAddress::Any,port)) {
                say("cannot bind port: " + QString::number(port));
                serv->deleteLater();
                exit(0);
                return false;
            }
            this->srv = serv;
            return true;
            break;
        }
        case ServerSocket::Udp: {
            sock = new QUdpSocket(this);
            if (!sock->bind(port)) {
                say("cannot bind port: " + QString::number(port));
                sock->deleteLater();
                exit(0);
                return false;
            }
            connect(sock,SIGNAL(readyRead()),this,SLOT(newConnection()));
            this->srv = sock;
            return true;
            break;
        }
        case ServerSocket::Invalid: {
            return false;
            break;
        }
    }
    return false;
}
void ServerSocket::newConnection() {
    if (currentType == Tcp) {
        say("sockopen.");
        QTcpSocket* sock = sockOpenTcp();
        ServerSecurity* security = qobject_cast<ServerMain*>(this->parent())->security;
        QHostAddress peer(sock->peerAddress());

        if (!security->isAuthorisedHost(&peer)) {
            const QString host = sock->peerAddress().toString();
            sock->close();
            sock->deleteLater();
            say("refused tcp incoming connection from " + host);
            return;
        }
        emit(sockOpen(sock));
    }
    else sockOpenUdp();

}
void ServerSocket::say(const QString message) {
    emit(debug(message));
}
QTcpSocket *ServerSocket::sockOpenTcp() {
    QTcpServer* tcp = (QTcpServer*) srv;
    QTcpSocket *sock = tcp->nextPendingConnection();
    return sock;
}

void ServerSocket::sockOpenUdp() {
    QUdpSocket* udp = (QUdpSocket*) srv;
    while (udp->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(udp->pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort;

        udp->readDatagram(datagram.data(),datagram.size(),&sender,&senderPort);
        emit(readData(&sender,&senderPort,&datagram,udp));
    }
}
QUdpSocket* ServerSocket::getUdpSocket() {
    if (currentType == Tcp) return 0;
    return (QUdpSocket*) this->srv;
}
ServerSocket::type ServerSocket::getServerType() {
    return currentType;
}
QString ServerSocket::typeToString(ServerSocket::type type) {
    switch (type) {
        case Udp:
            return QString("Udp");
        case Tcp:
            return QString("Tcp");
        case Invalid:
            return QString("Invalid");
    }
    return QString("Unknow");
}
