#include "user.h"
#include "server/servermain.h"
#include <QTime>
#include <QRegExp>

User::User(QObject *socket, ServerSocket::type type, QObject *parent) :
    QObject(parent)
{
    bytesRead = 0;
    this->connectionTime = QTime::currentTime().msec();
    this->sockType = type;
    this->sock = socket;
    if (type == ServerSocket::Tcp) {
        QTcpSocket* tcp = (QTcpSocket*) socket;
        connect(tcp,SIGNAL(stateChanged(QAbstractSocket::SocketState)),this,SLOT(sockStateChanged(QAbstractSocket::SocketState)));
        connect(tcp,SIGNAL(readyRead()),this,SLOT(sockRead()));
        this->setObjectName(tcp->peerAddress().toString());

    }
    else if (type == ServerSocket::Udp) {
        QUdpSocket* udp = (QUdpSocket*) this->sock;
        this->setObjectName(udp->peerAddress().toString());
    }

    //Creating the anti afk class
    this->afk = new AfkKiller(2000,this);

    //Starting the anti afk class
    this->afk->start();

    //creating the manager, this class will manage the output using the CircularDevice
    this->manager = new Manager(this);
    connect(this->manager,SIGNAL(debug(QString)),this,SIGNAL(debug(QString)));

    AudioFormat* format = new AudioFormat();


    mc.bufferSize = 0;
    mc.format = format;
    mc.format->setChannelCount(2);
    mc.format->setCodec("audio/pcm");
    mc.format->setSampleRate(96000);
    mc.format->setSampleSize(16);

    this->inputDevice = new CircularDevice(2097152,this);
    //this->inputDevice = new QBuffer(this);

    this->inputDevice->open(QIODevice::ReadWrite);

    mc.devIn = this->inputDevice;
    mc.modeInput = Manager::Raw;
    mc.modeOutput = Manager::Device;
#ifdef PULSE
    mc.modeOutput = Manager::PulseAudio;
#endif
    mc.devicesNames.output = this->objectName();
    //if the new user is localhost: let's just mute him (just to prevent ears/speakers to explode :p)
    if (this->objectName() == "127.0.0.1") mc.modeOutput = Manager::Zero;

    QByteArray specs = mc.format->getFormatTextInfo().toLocal8Bit();
    specs.append("afk:");
    specs.append(QString::number(afk->getInterval()));
    send(specs);
}
User::~User() {
    say("deleting object");
    if (!sockType == ServerSocket::Tcp) {
        QTcpSocket* sock = (QTcpSocket*) this->sock;
        sock->close();
        sock->deleteLater();
    }
    afk->deleteLater();
    manager->deleteLater();
    //we dont delete 'format' here because the manager will do this :)
}

void User::sockStateChanged(QAbstractSocket::SocketState state) {
    switch (state) {
        case QAbstractSocket::ConnectedState:
            say("connected");
            break;
        case QAbstractSocket::ClosingState:
            say("closing state");
            emit(sockClose(this));
            break;
        case QAbstractSocket::UnconnectedState:
            say("unconnected");
            break;
        case QAbstractSocket::ListeningState:
            say("listening");
            break;
        case QAbstractSocket::BoundState:
            say("bounding");
            break;
        case QAbstractSocket::HostLookupState:
            say("looking for host");
            break;
        case QAbstractSocket::ConnectingState:
            say("connecting...");
            break;
    }
}
void User::say(const QString message) {
    emit(debug("User: " + this->objectName() + ": " + message));
}
QString User::getUserName() {
    return this->sock->objectName();
}
void User::sockRead() {
    //this is the Tcp sockread, the udp data DONT cant this method
    //say("sockread !");
    QTcpSocket* sock = (QTcpSocket*) this->sock;
    QByteArray data = sock->readAll();

    const quint64 size = data.size();
    if (!bytesRead) {
        readUserConfig(&data);
        manager->setUserConfig(mc);
        manager->start();
        bytesRead += size;
        return;
    }
    inputDevice->write(data,size);
    bytesRead += size;
}
void User::sockRead(const QByteArray* data) {
    //this is a Udp sockread
    QUdpSocket* sock = (QUdpSocket*) this->sock;
    (void) sock;

    const int size = data->size();
    if (!bytesRead) {
        readUserConfig(data);
        manager->setUserConfig(mc);
        manager->start();
        bytesRead += size;
        return;
    }
    inputDevice->write(data->data(),size);
    bytesRead += size;
    emit(readedNewBytes(size));
}
void User::stop() {
    emit(sockClose(this));
    this->deleteLater();
}

bool User::readUserConfig(const QByteArray *data) {
    QString rawUserConfig = QString(*data).split("\n").first();
    QRegExp exp("[\\W]|[^\\d]|[^:]|[^\\s]",Qt::CaseSensitive,QRegExp::RegExp2);
    if (rawUserConfig.isEmpty()) say("no user config");
    else if (exp.indexIn(rawUserConfig)) say("no user config");
    else {
        say("readed user config: " + rawUserConfig);


        QStringList options = rawUserConfig.split(" ");
        QStringList::iterator i;
        for (i = options.begin();i != options.end();i++) {
            QString opt = *i;
            QStringList fields = opt.split(":");
            const int argc = fields.count();
            if (argc < 2) {
                if (!opt.isEmpty()) say("missing value for option: " + opt);
            }
            else {
                QString value;
                const QString key = fields.at(0);
                if (fields.count() >= 2) {
                    value = fields.at(1);
                }
                const int intVal = value.toInt();
                if (key == "samplerate") mc.format->setSampleRate(intVal);
                else if (key == "samplesize") mc.format->setSampleSize(intVal);
                else if (key == "name") this->setObjectName(value);
                else if (key == "channels") mc.format->setChannelCount(intVal);
                else send(QString("unknow option: " + key + "value: " + value).toLocal8Bit());
            }
        }
        say("end of user configuration received.");
        send(QString("configuration received !").toLocal8Bit());
        return true;
    }
    return false;
}
void User::send(const QByteArray data) {
    if (sockType == ServerSocket::Tcp) {
        QTcpSocket* sock = (QTcpSocket*) this->sock;
        sock->write(data + (char) 10);
    }
    else {
        QUdpSocket* sock = (QUdpSocket*) this->sock;
        if (!sock->isOpen()) return;
        sock->write(data + (char) 10);
    }
}
void User::kill(const QString reason) {
    send(QString("you where kicked: reason: " + reason).toLocal8Bit());
    QTcpSocket* sock = (QTcpSocket*) this->sock;
    sock->close();
    emit(kicked());
}
const QObject* User::getSocketPointer() {
    return this->sock;
}
quint64 User::getBytesCount() {
    return bytesRead;
}
QHostAddress User::getHostAddress() {
    QHostAddress address;
    if (sockType == ServerSocket::Udp) address.setAddress(qobject_cast<QUdpSocket*>(this->sock)->peerAddress().toIPv4Address());
    else if (sockType == ServerSocket::Tcp) address.setAddress(qobject_cast<QTcpSocket*>(this->sock)->peerAddress().toIPv4Address());
    return address;
}
