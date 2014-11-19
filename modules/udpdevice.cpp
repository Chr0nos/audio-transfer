#include "udpdevice.h"
#include <QtNetwork/QHostInfo>

//this module can work in WriteOnly mode but no others: Udp dont use a connection, it just send data over the network and... thats all

UdpDevice::UdpDevice(const QString host, const int port, AudioFormat *format, const bool sendConfig, QObject *parent) :
    QIODevice(parent)
{
    this->host= host;
    this->port = port;
    this->format = format;
    sock = new QUdpSocket(this);
    bSendConfig = sendConfig;
}
bool UdpDevice::open(OpenMode mode) {
    say("opening device...");
    if ((mode == QIODevice::WriteOnly || (mode == QIODevice::ReadWrite))) {
        sock->connectToHost(host,port,mode);
        if (sock->waitForConnected()) {
            say("connected to remote host");
            QString name = this->objectName();
            if (name.isEmpty()) name = QHostInfo::localHostName();

            QIODevice::open(mode);
            QByteArray specs = format->getFormatTextInfo().toLocal8Bit();
            specs.append(" name:" + name);
            sock->write(specs);
            sock->flush();
            return true;
        }
        say("connection error: " + sock->errorString());
        return false;
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
    (void) data;
    if (sock->isReadable()) data = sock->read(maxlen).data();
    return maxlen;
}
void UdpDevice::close() {
    QIODevice::close();
    if (sock->isOpen()) sock->close();
    say("device closed");
}
void UdpDevice::say(const QString message) {
#ifdef DEBUG
    qDebug() << "UdpDevice : " + message;
#endif
    emit(debug("UdpDevice: " + message));
}

