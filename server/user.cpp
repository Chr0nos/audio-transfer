#include "user.h"
#include "server/servermain.h"
#include <QTime>
#include <QRegExp>

User::User(QObject *socket, ServerSocket::type type, QObject *parent) :
    QObject(parent)
{
    bytesRead = 0;
    this->managerStarted = false;
    this->connectionTime = QTime::currentTime().msec();
    this->sockType = type;
    this->sock = socket;
    lastBytesRead = 0;
    this->flowChecker = NULL;
    if (type == ServerSocket::Tcp) {
        QTcpSocket* tcp = (QTcpSocket*) socket;
        connect(tcp,SIGNAL(stateChanged(QAbstractSocket::SocketState)),this,SLOT(sockStateChanged(QAbstractSocket::SocketState)));
        connect(tcp,SIGNAL(readyRead()),this,SLOT(sockRead()));
        this->setObjectName(tcp->peerAddress().toString());

    }

    //at this point, the object name IS the peer address
    this->peerAddress = this->objectName();

    //creating the manager, this class will manage the output using the CircularDevice
    this->manager = new Manager(this);
    //connect(this->manager,SIGNAL(debug(QString)),this,SIGNAL(debug(QString)));

    //flow checker interval
    //note: the flow checker also check for afk users.
    checkInterval = 2000;

    mc.bufferSize = 0;

    //creating the default audio format
    mc.format = new AudioFormat();
    mc.format->setChannelCount(2);
    mc.format->setCodec("audio/pcm");
    mc.format->setSampleRate(96000);
    mc.format->setSampleSize(16);

    //creating a 2Mb ring buffer device
    this->inputDevice = new CircularDevice(2097152,this);
    //this->inputDevice = new QBuffer(this);

    //opening the buffer and defining it for the manager
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
    specs.append(QString::number(checkInterval));
    send(specs);
}
User::~User() {
    say("deleting object");
    if (sockType == ServerSocket::Tcp) {
        QTcpSocket* sock = (QTcpSocket*) this->sock;
        sock->close();
        sock->deleteLater();
    }
    //in udp you MUST dont close the socket.

    if (flowChecker) flowChecker->deleteLater();
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
    emit(debug("User: " + this->getUserName() + ": " + message));
}
QString User::getUserName() {
    return this->objectName();
}
void User::sockRead() {
    //this is the Tcp sockread, the udp data DONT cant this method
    //say("sockread !");
    QTcpSocket* sock = (QTcpSocket*) this->sock;
    QByteArray data = sock->readAll();

    const quint64 size = data.size();
    if ((!bytesRead) && (!managerStarted)) {
        readUserConfig(&data);
        initUser();
        bytesRead += size;
        return;
    }

    inputDevice->write(data,size);
    bytesRead += size;
}
void User::initUser() {
    mc.devicesNames.output = this->objectName();
    manager->setUserConfig(mc);
    managerStarted = manager->start();

    //creating the flow checker (it check if the data are comming at the good speed)
    this->flowChecker = new FlowChecker(mc.format,this->checkInterval,this);
    connect(flowChecker,SIGNAL(debug(QString)),this,SLOT(say(QString)));
    flowChecker->start();
}

void User::sockRead(const QByteArray* data) {
    //this is a Udp sockread
    QUdpSocket* sock = (QUdpSocket*) this->sock;
    (void) sock;
    const int size = data->size();

    if ((!bytesRead) && (!managerStarted)) {
        readUserConfig(data);
        initUser();
        bytesRead += size;
        return;
    }
    inputDevice->write(data->data(),size);
    bytesRead += size;
}
void User::stop() {
    emit(sockClose(this));
}

bool User::readUserConfig(const QByteArray *data) {
    QString rawUserConfig = QString(*data).split("\n").first();
    QRegExp exp("[\\W]|[^\\d]|[^:]|[^\\s]",Qt::CaseSensitive,QRegExp::RegExp2);
    if (rawUserConfig.isEmpty()) say("no user config");
    else if (exp.indexIn(rawUserConfig)) say("no user config");
    else {
        Readini* ini = qobject_cast<ServerMain*>(this->parent())->getIni();
        if (!ini->isKey("general","userConfig"));
        else if (!ini->getValue("general","userConfig").toInt()) {
            say("refused user config: not allowed in the configuration file.");
            return false;
        }
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
                else if (key == "name") {
                    say("renaming user to: " + value);
                    this->setObjectName(value);
                }
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
    if (sockType == ServerSocket::Tcp) {
        QTcpSocket* sock = (QTcpSocket*) this->sock;
        sock->close();
    }
    flowChecker->stop();
    emit(kicked());
}
void User::ban(const QString reason, const int banTime) {
    QHostAddress host = getHostAddress();
    callSecurity()->addToBannedList(&host,banTime);

    kill(reason);
}
ServerSecurity* User::callSecurity() {
    return qobject_cast<UserHandler*>(this->parent())->callSecurity();
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
int User::getSpeed() {
    //the speed will be returned in bytes per seconds
    lastBytesRead = bytesRead;
    const int elaspedTime = speedLastCheckTime.elapsed();
    if (!elaspedTime) return -1;

    const int speed = (bytesRead - lastBytesRead) / elaspedTime;

    speedLastCheckTime = QTime::currentTime();
    lastBytesRead = bytesRead;
    return speed;
}
