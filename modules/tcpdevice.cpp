/* this module will just be an output for playback over an other audio server (like a relay)
 */

#include "tcpdevice.h"
#include "audioformat.h"
#include <QString>
#include <QIODevice>
#include <QDebug>

TcpDevice::TcpDevice(const QString host, const int port, AudioFormat *format,bool sendConfig,QObject *parent) :
    QIODevice(parent)
{
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
}
bool TcpDevice::open(OpenMode mode) {
    if (mode == QIODevice::WriteOnly) {
        sock->connectToHost(host,port,mode);
        sock->waitForConnected();
        if (sock->isOpen()) {
            QIODevice::open(mode);
            return true;
        }
    }
    return false;
}
qint64 TcpDevice::readData(char *data, qint64 maxlen) {
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
    sock->write(format->getFormatTextInfo().toLocal8Bit());
    sock->flush();
}
void TcpDevice::sockClose() {
    this->close();
}
void TcpDevice::sockOpen() {
   if (bSendConfig) sendFormatSpecs();
}
void TcpDevice::say(const QString message) {
    qDebug() << "TcpDevice: " + message;
}
void TcpDevice::close() {
    say("closing device");
    QIODevice::close();
    sock->close();
}
