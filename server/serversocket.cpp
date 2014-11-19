#include "serversocket.h"
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QUdpSocket>

ServerSocket::ServerSocket(QObject *parent) :
    QObject(parent)
{
}
bool ServerSocket::startServer(ServerSocket::type type, const int port) {
    this->currentType = type;
    switch (type) {
        case ServerSocket::Tcp: {
            QTcpServer* serv = new QTcpServer(this);
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
            QUdpSocket *sock = new QUdpSocket(this);
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
    }
    return false;
}
void ServerSocket::newConnection() {
    if (currentType == Tcp) {
        say("sockopen.");
        emit(sockOpen(sockOpenTcp()));
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
