/* this module will just be an output for playback over an other audio server (like a relay)
 * Todo: test the read mode (the server has to send audio to the client (like a microphone return)
 */

#include "tcpdevice.h"
#include "audioformat.h"
#include <QString>
#include <QIODevice>
#include <QDebug>

TcpDevice::TcpDevice(const QString host, const int port, AudioFormat *format,bool sendConfig,QObject *parent) :
    QIODevice(parent)
{
    say("init");
    (void) sendConfig;
    this->port = port;
    this->host = host;
    this->sock = new QTcpSocket(this);
    sock->setSocketOption(QAbstractSocket::LowDelayOption,0);
    connect(sock,SIGNAL(disconnected()),this,SLOT(sockClose()));
    connect(sock,SIGNAL(disconnected()),this,SIGNAL(aboutToClose()));
    connect(sock,SIGNAL(connected()),this,SLOT(sockOpen()));
    connect(sock,SIGNAL(readyRead()),this,SIGNAL(readyRead()));
    this->format = format;
    bSendConfig = true;
    say("init done");
}
bool TcpDevice::open(OpenMode mode) {
    say("opening device (connect)");
    sock->connectToHost(host,port,mode);
    if (sock->waitForConnected()) {
        say("socket open");
        QIODevice::open(mode);
        return true;
    }
    say("connection timed out, check target ip and if server is runing");
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
        sock->write(data,len);
        return len;
    }
    return -1;
}
void TcpDevice::sendFormatSpecs() {
    say("sending format specs");
    sock->write(format->getFormatTextInfo().toLocal8Bit());
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

