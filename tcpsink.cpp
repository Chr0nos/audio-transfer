#include "tcpsink.h"
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QAbstractSocket>
#include <QTextStream>
#include <QDebug>

TcpSink::TcpSink(QObject *parent) :
    QObject(parent)
{
}
TcpSink::~TcpSink() {
    sock->deleteLater();
}
void TcpSink::connectToHost(const QString targetAddress, const int targetPort) {
    host = targetAddress;
    port = targetPort;
    devOut = 0;
    sock = new QTcpSocket(this);
    sock->setSocketOption(QAbstractSocket::LowDelayOption,0);
    connect(sock,SIGNAL(connected()),this,SIGNAL(connected()));
    connect(sock,SIGNAL(readyRead()),this,SIGNAL(readyWrite()));
    connect(sock,SIGNAL(aboutToClose()),this,SIGNAL(disconnected()));
    connect(sock,SIGNAL(destroyed()),this,SIGNAL(disconnected()));
    connect(sock,SIGNAL(stateChanged(QAbstractSocket::SocketState)),this,SLOT(sockEvents(QAbstractSocket::SocketState)));

    sock->connectToHost(host,port);
    sock->waitForConnected();
    devOut = sock;
}
void TcpSink::disconnectFromHost() {
    if (sock->isOpen()) sock->disconnectFromHost();
}

QIODevice* TcpSink::getDevice() {
    return devOut;
}
void TcpSink::send(QString *message) {
    if (!sock->isOpen()) return;
    QTextStream out(sock);
    qDebug() << "sending: " << *message;
    out << *message << endl;
}
void TcpSink::sockEvents(QAbstractSocket::SocketState event) {
    if (event == QAbstractSocket::ClosingState) emit(disconnected());
    else if (event == QAbstractSocket::ConnectedState) emit(connected());
    //qDebug() << "event: " << event;
}
