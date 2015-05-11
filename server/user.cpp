#include "user.h"
#include "server/servermain.h"
#include <QTime>
#include <QMutexLocker>

/*
** Todo: implement security directly in this user class
*/

User::User(QObject *socket, ServerSocket::type type, QObject *parent) :
    QObject(parent)
{
    this->ini = qobject_cast<UserHandler*>(this->parent())->getIni();
    this->security = qobject_cast<UserHandler*>(this->parent())->callSecurity();
    bytesRead = 0;
    this->managerStarted = false;
    this->connectionTime = QTime::currentTime().msec();
    this->sockType = type;
    this->sock = socket;
    lastBytesRead = 0;
    this->flowChecker = NULL;
    this->type = type;
    this->manager = NULL;
    this->mutex = NULL;

    //at this point, the object name IS the peer address
    this->peerAddress = this->objectName();

    if (ini->getValue("general","verbose").toInt()) connect(this->manager, SIGNAL(debug(QString)), this, SIGNAL(debug(QString)));

    //flow checker interval
    //note: the flow checker also check for afk users.
    checkInterval = 2000;

    //because the server works in local mode we dont need a buffer
    mc.bufferSize = 0;

    if (!ini->isKey("general","userConfig")) this->allowUserConfig = true;
    else this->allowUserConfig = (bool) ini->getValue("general","userConfig").toInt();

    moduleName = ini->getValue("general","output");
}

void User::start()
{
    this->mutex = new QMutex();
    //QMutexLocker lock(this->mutex);
    say("user start...");
    if (type == ServerSocket::Tcp)
    {
        QTcpSocket *tcp = (QTcpSocket*) socket;
        tcp->setParent(this);
        connect(tcp,SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(sockStateChanged(QAbstractSocket::SocketState)));
        connect(tcp,SIGNAL(readyRead()),this,SLOT(sockRead()));
        this->setObjectName(tcp->peerAddress().toString());
    }

    say("user creating device");
    //creating a 2Mb ring buffer device
    this->inputDevice = new CircularDevice(2097152, this);
    this->inputDevice->open(QIODevice::ReadWrite);
    mc.raw.devIn = this->inputDevice;

    say("device done");
    //confiruring output module
    initModule();

    //creating the default audio format
    initFormat();

    //creating the manager, this class will manage the output using the CircularDevice
    this->manager = new Manager(this);
    sendSpecs();

    say("user is now ready");
}

void User::sendSpecs()
{
    /*
    ** this method send the server specification to
    ** the remote client, must be called AFTER mc.format initialisation
    ** called by: this->start();
    */
    QByteArray specs = mc.format->getFormatTextInfo().toLocal8Bit();
    specs.append("afk:");
    specs.append(QString::number(checkInterval));
    send(&specs);
}

void User::initModule()
{
    /*
    ** called by: this->start();
    */
    //opening the buffer and defining it for the manager
    mc.modeInput = Manager::Raw;
    mc.modeOutput = Manager::getModeFromString(&moduleName);
    //this could appens if the configuration file was not configured or badly configured, so we load the "default" output
    if (mc.modeOutput == Manager::None)
    {
        #ifdef PULSE
            mc.modeOutput = Manager::PulseAudio;
        #else
            mc.modeOutput = Manager::Device;
        #endif
    }
    //if the new user is localhost: let's just mute him (just to prevent ears/speakers to explode :p)
    if (this->objectName() == "127.0.0.1") mc.modeOutput = Manager::Zero;
}

User::~User()
{
    //QMutexLocker lock(this->mutex);
    say("deleting object");
    if (sockType == ServerSocket::Tcp)
    {
        QTcpSocket *sock = (QTcpSocket*) this->sock;
        if (sock->isOpen()) sock->close();
        sock->disconnect();
        sock->deleteLater();
    }
    //in udp you MUST dont close the socket.
    if (flowChecker)
    {
        flowChecker->stop();
        flowChecker->disconnect();
        delete(flowChecker);
    }
    if (manager)
    {
        manager->disconnect();
        delete(manager);
    }
    //lock.unlock();
    delete(this->mutex);
    //we dont delete 'format' here because the manager will do this :)
}

void User::initFormat()
{
    short channels;
    int rate;
    short size;
    QString codec;

    channels = 0;
    rate = 0;
    size = 0;
    if ((ini) && (ini->exists()))
    {
        channels = ini->getValue("format", "channels").toInt();
        rate = ini->getValue("format", "sampleRate").toInt();
        codec = ini->getValue("format", "codec");
        size = ini->getValue("format", "sampleSize").toInt();
    }
    if (channels <= 0) channels = 2;
    if (!rate) rate = 96000;
    if (codec.isEmpty()) codec = "audio/pcm";
    if (!size) size = 16;
    mc.format = new AudioFormat();
    mc.format->setChannelCount(channels);
    mc.format->setCodec(codec);
    mc.format->setSampleRate(rate);
    mc.format->setSampleSize(size);
}

void User::sockStateChanged(QAbstractSocket::SocketState state) {
    switch (state)
    {
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

void User::say(const QString message)
{
    emit(debug("User: " + this->getUserName() + ": " + message));
}

QString User::getUserName()
{
    return this->objectName();
}

void User::sockRead()
{
    //this is the Tcp sockread, the udp data DONT cant this method
    //say("sockread !");
    //QMutexLocker lock(this->mutex);
    QTcpSocket* sock = (QTcpSocket*) this->sock;
    QByteArray data = sock->readAll();
    const quint64 size = data.size();

    if ((!bytesRead) && (!managerStarted))
    {
        readUserConfig(&data);
        initUser();
        bytesRead += size;
        return;
    }
    inputDevice->write(data, size);
    bytesRead += size;
}

void User::initUser()
{
    this->mc.devicesNames.output = this->objectName();
    this->manager->setUserConfig(this->mc);
    this->managerStarted = this->manager->start();
    this->initFlowChecker();
}

void User::initFlowChecker()
{
    QMutexLocker lock(this->mutex);

    //creating the flow checker (it check if the data are comming at the good speed)
    this->flowChecker = new FlowChecker(mc.format,this->checkInterval,this);
    connect(flowChecker,SIGNAL(ban(QString,int)),this,SLOT(ban(QString,int)));
    connect(flowChecker,SIGNAL(kick(QString)),this,SLOT(kill(QString)));
    connect(flowChecker,SIGNAL(debug(QString)),this,SLOT(say(QString)));
    flowChecker->start();
}

void User::sockRead(const QByteArray *data)
{
    //this is a Udp sockread
    //QUdpSocket* sock = (QUdpSocket*) this->sock;
    //(void) sock;
    const int size = data->size();

    //QMutexLocker lock(this->mutex);
    if ((!bytesRead) && (!managerStarted))
    {
        readUserConfig(data);
        initUser();
        bytesRead += size;
        return;
    }
    inputDevice->write(data->data(),size);
    bytesRead += size;
}

void User::readData(QHostAddress *sender, const quint16 *senderPort, const QByteArray *data, QUdpSocket *udp)
{
    /*
    ** this slot is used to be connected directly with
    ** the serversocket class who read directly the udp datagram
    */
    (void) sender;
    (void) senderPort;
    (void) udp;
    this->sockRead(data);
}

void User::stop()
{
    QMutexLocker lock(this->mutex);
    if (this->inputDevice)
    {
        this->inputDevice->close();
    }
    emit(sockClose(this));
}

bool User::isPossibleConfigLine(const char *input, int lenght)
{
    /*
     ** this function check if a config line could be a valid one or not
     ** it's here to replace the regex usage and be more acurate
     ** allowed range are: a-z A-Z : 0-9
     */
    int i;
    char c;

    i = 0;
    while (i < lenght)
    {
        c = input[i];
        if (c == 32);
        else if (c == ':');
        else if ((c >= 'A') && (c <= 'Z'));
        else if ((c >= 'a') && (c <= 'z'));
        else if ((c >= '0') && (c <= '9'));
        else return false;
        i++;
    }
    return true;
}

bool User::readUserConfig(const QByteArray *data)
{
    /*
    ** this method is called if the user
    ** has not sent anything yet
    ** it detect if the user is sending  a configuration line
    ** and it parse parameters sent by the remote client
    */
    //QMutexLocker lock(this->mutex);
    QString rawUserConfig = QString(*data).split("\n").first();
    if (!isPossibleConfigLine(rawUserConfig.toLocal8Bit().data(), rawUserConfig.length()))
    {
        say("no user config");
    }
    else
    {
        int intVal;
        QString key;
        QString value;
        QString opt;
        QStringList fields;
        int argc;
        QStringList options;
        QStringList::iterator i;
        QByteArray confirm;

        confirm = QString("configuration received !").toLocal8Bit();

        if (!this->allowUserConfig)
        {
            say("refused user config: not allowed in the configuration file.");
            return false;
        }
        say("readed user config: " + rawUserConfig);
        options = rawUserConfig.split(" ");
        for (i = options.begin() ; i != options.end() ; i++) {
            opt = *i;
            fields = opt.split(":");
            argc = fields.count();
            if ((argc < 2) && (!opt.isEmpty())) say("missing value for option: " + opt);
            else
            {
                value.clear();
                key = fields.at(0);
                if (argc >= 2)
                {
                    value = fields.at(1);
                }
                intVal = value.toInt();
                readUserConfigOption(&key, &value, &intVal);
            }
        }
        say("end of user configuration received.");
        send(&confirm);
        return true;
    }
    return false;
}

bool User::readUserConfigOption(const QString *key, const QString *value, const int *intVal)
{
    /*
    ** this method is the inside of the user line configuration
    ** parser, any other option should be added here
    */
    QByteArray errorString;

    if (*key == "samplerate") {
        if (*intVal < 1)
        {
            kill("invalid sample rate.");
            return true;
        }
        else mc.format->setSampleRate(*intVal);
    }
    else if (*key == "samplesize") mc.format->setSampleSize(*intVal);
    else if (*key == "name")
    {
        if (value->length() > 64) say("rejecting user name: name is too long.");
        else
        {
            say("renaming user to: " + *value);
            this->setObjectName(*value);
        }
    }
    else if (*key == "channels")
    {
        if ((!*intVal) || (*intVal < 1))
        {
            say("user sent invalid number of channels: closing connection.");
            this->kill("invalid channels numbers");
            return true;
        }
        mc.format->setChannelCount(*intVal);
    }
    else
    {
        errorString = QString("unknow option: " + *key + "value: " + *value).toLocal8Bit();
        send(&errorString);
    }
    return false;
}

void User::send(QByteArray *data)
{
    QTcpSocket *tcp;
    QUdpSocket *udp;
    quint64 size;

    data->append((char) 10);
    size = data->size();
    if (sockType == ServerSocket::Tcp)
    {
        tcp = (QTcpSocket*) this->sock;
        tcp->write(data->data(), size);
    }
    else
    {
        udp = (QUdpSocket*) this->sock;
        if (!udp->isOpen()) return;
        udp->write(data->data(), size);
    }
}

void User::kill(const QString reason)
{
    QByteArray reason_b;

    reason_b = QString("you where kicked: reason: " + reason).toLocal8Bit();
    flowChecker->setParent(NULL);
    say("kicking user: " + reason);
    send(&reason_b);
    if (sockType == ServerSocket::Tcp)
    {
        QTcpSocket* sock = (QTcpSocket*) this->sock;
        sock->close();
    }
    flowChecker->stop();
    emit(kicked());
}

void User::ban(const QString reason, const int banTime)
{
    /*
    ** this method ban the current user with the given reason
    ** for the banTime
    ** if banTime = 0 : the ban is permanent
    */
    QHostAddress host;

    host = getHostAddress();
    callSecurity()->addToBannedList(&host, banTime);
    kill(reason);
}

ServerSecurity* User::callSecurity()
{
    return security;
}

const QObject* User::getSocketPointer()
{
    return this->sock;
}

quint64 User::getBytesCount()
{
    return bytesRead;
}

QHostAddress User::getHostAddress()
{
    QHostAddress address;

    if (sockType == ServerSocket::Udp) address.setAddress(qobject_cast<QUdpSocket*>(this->sock)->peerAddress().toIPv4Address());
    else if (sockType == ServerSocket::Tcp) address.setAddress(qobject_cast<QTcpSocket*>(this->sock)->peerAddress().toIPv4Address());
    return address;
}

int User::getSpeed()
{
    int elaspedTime;
    int speed;

    //the speed will be returned in bytes per seconds
    lastBytesRead = bytesRead;
    elaspedTime = speedLastCheckTime.elapsed();
    if (!elaspedTime) return -1;

    speed = (bytesRead - lastBytesRead) / elaspedTime;

    speedLastCheckTime = QTime::currentTime();
    lastBytesRead = bytesRead;
    return speed;
}

Readini* User::getIni()
{
    return ini;
}

void User::moveToThread(QThread *thread)
{
    /*
    ** for now: this method make the whole application has
    ** a segmentation fault, so don't move to threads yet
    ** i'm working on it
    */
    say("moving to an other thread");
    connect(thread, SIGNAL(finished()), this, SLOT(deleteLater()));
    QObject::moveToThread(thread);
    /*
    if (type == ServerSocket::Tcp)
    {
        this->sock->moveToThread(thread);
    }
    this->flowChecker->moveToThread(thread);
    this->manager->moveToThread(thread);
    this->inputDevice->moveToThread(thread);
    */
    say("moving done");
}
