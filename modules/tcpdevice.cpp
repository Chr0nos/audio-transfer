/* this module will just be an output for playback over an other audio server (like a relay)
 * Todo: test the read mode (the server has to send audio to the client (like a microphone return)
 */

#include "tcpdevice.h"
#include "audioformat.h"
#include "manager.h"
#include <QString>
#include <QIODevice>
#include <QtNetwork/QHostInfo>
#include <QDebug>

TcpDevice::TcpDevice(const QString host, const int port, AudioFormat *format,bool sendConfig,QObject *parent) :
    ModuleDevice(parent)
{
    say("init");
    this->port = port;
    this->host = host;
    this->sock = new QTcpSocket(this);
    sock->setObjectName("TcpDevice module");
    sock->setSocketOption(QAbstractSocket::LowDelayOption,0);
    connect(sock,SIGNAL(disconnected()),this,SLOT(sockClose()));
    connect(sock,SIGNAL(disconnected()),this,SIGNAL(aboutToClose()));
    connect(sock,SIGNAL(connected()),this,SLOT(sockOpen()));
    connect(sock,SIGNAL(readyRead()),this,SIGNAL(readyRead()));
    connect(sock,SIGNAL(stateChanged(QAbstractSocket::SocketState)),this,SLOT(stateChanged(QAbstractSocket::SocketState)));
    this->format = format;
    bSendConfig = sendConfig;
    say("init done");
}

bool TcpDevice::open(OpenMode mode) {
    say("opening device (connect)");
    say("connecting to: " + host + " on port: " + QString::number(port));
    sock->connectToHost(host,port,mode);
    if (sock->waitForConnected()) {
        say("socket open");
        QIODevice::open(mode);
        return true;
    }
    //say("connection timed out, check target ip and if server is runing");
    say("connection error: " + sock->errorString());
    emit(sockClose());
    return false;
}

qint64 TcpDevice::readData(char *data, qint64 maxlen) {
    (void) data;
    if (sock->isReadable()) {
        data = sock->read(maxlen).data();
        return maxlen;
    }
    return -1;
}

qint64 TcpDevice::writeData(const char *data, qint64 len) {
    if (sock->isWritable()) {
        return sock->write(data,len);
    }
    return -1;
}

void TcpDevice::sendFormatSpecs() {
    say("sending format specs");
    QString name = this->objectName();
    if (name.isEmpty()) name = QHostInfo::localHostName();
    QByteArray specs = format->getFormatTextInfo().toLocal8Bit();
    specs.append(" name:" + name);

    sock->write(specs);
    sock->flush();
}

void TcpDevice::sockClose() {
    say("socket closed");
    this->close();
}

void TcpDevice::sockOpen() {
   say("connected to remote server");
   if (bSendConfig) sendFormatSpecs();
}

void TcpDevice::say(const QString message) {    
    emit(debug("TcpDevice: " + message));
}

void TcpDevice::close() {
    say("closing device");
    QIODevice::close();
    sock->close();
}

void TcpDevice::stateChanged(QAbstractSocket::SocketState state) {
    switch (state) {
        case QAbstractSocket::UnconnectedState:
            say("not connected");
            break;
        case QAbstractSocket::HostLookupState:
            say("looking for target host");
            break;
        case QAbstractSocket::ConnectingState:
            say("connecting");
            break;
        case QAbstractSocket::BoundState:
            say("bound state");
            break;
        case QAbstractSocket::ListeningState:
            say("listening");
            break;
        case QAbstractSocket::ConnectedState:
            say("connected");
            break;
        case QAbstractSocket::ClosingState:
            say("closing");
            break;
    }
}

qint64 TcpDevice::bytesAvailable() {
    if (!sock) return 0;
    return sock->bytesAvailable() + QIODevice::bytesAvailable();
}

ModuleDevice* TcpDevice::factory(QString name, AudioFormat *format, void *userData, QObject *parent)
{
    Manager::userConfig *config;
    TcpDevice *device;

    config = (Manager::userConfig*) userData;
    device = new TcpDevice(config->network.host, config->network.port, format, true, parent);
    device->setObjectName(name);
    return device;
}
