#include "udpdevice.h"

UdpDevice::UdpDevice(const QString host, const int port, AudioFormat *format, const bool sendConfig, QObject *parent) :
    QIODevice(parent)
{
    this->host= host;
    this->port = port;
    this->format = format;
    sock = new QUdpSocket(this);
    connect(sock,SIGNAL(disconnected()),this,SLOT(sockClose()));
    connect(sock,SIGNAL(connected()),this,SLOT(sockOpen()));
    bSendConfig = sendConfig;
}
bool UdpDevice::open(OpenMode mode) {
    if ((mode == QIODevice::WriteOnly || (mode == QIODevice::ReadWrite))) {
        sock->connectToHost(host,port,mode);
        sock->waitForConnected();
        QIODevice::open(mode);
        if (sock->isWritable()) {
            sock->write(format->getFormatTextInfo().toLocal8Bit());
            sock->flush();
            return true;
        }
    }
    else say("unsuported mode");
    return false;
}
qint64 UdpDevice::writeData(const char *data, qint64 len) {
    if (sock->isWritable()) {
        sock->write(data,len);
        return len;
    }
    return -1;
}
qint64 UdpDevice::readData(char *data, qint64 maxlen) {
    if (sock->isReadable()) data = sock->read(maxlen).data();
    return maxlen;
}
void UdpDevice::sockClose() {
    say("socket closed");
    this->close();
}
void UdpDevice::sockOpen() {
    say("connected to target");
    if (bSendConfig) sock->write(format->getFormatTextInfo().toLocal8Bit());
}

void UdpDevice::close() {
    QIODevice::close();
    if (sock->isOpen()) sock->close();
    say("device closed");
}
void UdpDevice::say(const QString message) {
    qDebug() << "UdpDevice : " + message;
}
