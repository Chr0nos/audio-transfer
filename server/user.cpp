#include "user.h"
#include "size.h"
#include "server/servermain.h"
#include <QTime>
#include <QMutexLocker>

/*
** Todo: implement security directly in this user class
**      dont use this->ini anymore to allow threads
**      use signals/slots for security for the same reasons
**
** this class manage ONE user
** it have it's own Manager who manage the audio output
** to write audio the class has just to write into
** the buffer "this->inputDevice" , this one is directly connected to
** the manager
*/

User::User(QObject *socket, ServerSocket::type type, QObject *parent) :
    QObject(parent)
{
    this->ini = qobject_cast<UserHandler*>(this->parent())->getIni();
    this->security = qobject_cast<UserHandler*>(this->parent())->callSecurity();
    this->bytesRead = 0;
    this->managerStarted = false;
    this->connectionTime = QTime::currentTime();
    this->sockType = type;
    this->sock = socket;
    this->lastBytesRead = 0;
    this->flowChecker = NULL;
    this->manager = NULL;
    this->mutex = NULL;
    this->pendingBuffer = NULL;
    this->isThreaded = false;

    //at this point, the object name IS the peer address
    this->peerAddress = this->objectName();

    if (this->ini->getValue("general", "verbose").toInt())
    {
        connect(this->manager, SIGNAL(debug(QString)), this, SIGNAL(debug(QString)));
    }

    //flow checker interval
    //note: the flow checker also check for afk users.
    this->checkInterval = 2000;

    //because the server works in local mode we dont need a buffer
    this->mc.bufferSize = 0;

    if (!this->ini->isKey("general", "userConfig")) this->allowUserConfig = true;
    else this->allowUserConfig = (bool) ini->getValue("general", "userConfig").toInt();

    this->moduleName = this->ini->getValue("general", "output");
}

void User::start()
{
    QTcpSocket *tcp;

    this->mutex = new QMutex();
    QMutexLocker lock(this->mutex);
    say("user start...");
    if (this->sockType == ServerSocket::Tcp)
    {
        tcp = (QTcpSocket*) this->sock;
        //tcp->setParent(this);
        connect(tcp, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(sockStateChanged(QAbstractSocket::SocketState)));
        connect(tcp, SIGNAL(readyRead()),this,SLOT(sockRead()));
        this->setObjectName(tcp->peerAddress().toString());
    }

    say("user creating device");
    //creating a 2Mb ring buffer device

    this->inputDevice = new CircularDevice(2097152, this);
    this->inputDevice->open(QIODevice::ReadWrite);
    this->mc.raw.devIn = this->inputDevice;

    say("device done");
    //confiruring output module
    this->initModule();

    //creating the default audio format
    this->initFormat();

    //creating the manager, this class will manage the output using the CircularDevice
    this->manager = new Manager(this);
    this->sendSpecs();

    say("user is now ready");

    flushPendingBuffer();
}

void User::flushPendingBuffer()
{
    /*
    ** this method flush the pending buffer
    ** the pending buffer is used to read sound before the User object is started
    ** it's usefull for threaded use
    */
    if (!this->pendingBuffer.isEmpty())
    {
        say("user has pending buffer: flushing it");
        this->mutex->unlock();
        this->sockReadInternal(&this->pendingBuffer, this->pendingBuffer.size());
        this->pendingBuffer.clear();
    }
}

void User::sendSpecs()
{
    /*
    ** this method send the server specification to
    ** the remote client, must be called AFTER mc.format initialisation
    ** called by: this->start();
    */
    QByteArray specs = mc.format->getFormatTextInfo().toLocal8Bit();
    specs.append(" afk:");
    specs.append(QString::number(checkInterval));
    this->send(&specs);
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
    QTcpSocket *tcp;

    say("deleting object");
    if (sockType == ServerSocket::Tcp)
    {
        tcp = (QTcpSocket*) this->sock;
        if (tcp->isOpen()) tcp->close();
        tcp->disconnect();
        tcp->deleteLater();
        this->sock = NULL;
    }
    //in udp you MUST dont close the socket.
    if (this->flowChecker)
    {
        this->flowChecker->stop();
        this->flowChecker->disconnect();
        delete(this->flowChecker);
        this->flowChecker = NULL;
    }
    if (this->manager)
    {
        this->manager->stop();
        this->manager->disconnect();
        delete(this->manager);
        this->manager = NULL;
    }
    if (this->mutex)
    {
        delete(this->mutex);
        this->mutex = NULL;
    }
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

void User::sockRead(const QByteArray *data)
{
    /*
    ** this is a Udp sockread slot
    ** in case of threaded user we have to copy the data
    ** because else: they will not be available anymore at the read moment
    */
    const int size = data->size();

    if (!this->isThreaded)
    {
        this->sockReadInternal(data, size);
    }
    else
    {
        this->sockReadInternalCopy(data, size);
    }
}

void User::sockRead()
{
    //this is the Tcp sockread, the udp data DONT call this method
    //say("sockread !");
    QByteArray data;
    quint64 size;
    QTcpSocket *tcp;

    tcp = (QTcpSocket*) this->sock;
    data = tcp->readAll();
    size = data.size();
    this->sockReadInternal(&data, size);
}

void User::sockReadInternal(const QByteArray *data, const int size)
{
    /*
    ** this method is called by both Tcp and Udp protocols
    ** if the manager is available the sound is direcly interpreted
    ** in other case: datas are placed into a pending buffer
    */
    QMutexLocker lock(this->mutex);
    if (!this->manager)
    {
        this->appendToPendingBuffer(data, size);
        return;
    }
    if ((!this->bytesRead) && (!this->managerStarted))
    {
        readUserConfig(data);
        initUser();
    }
    this->inputDevice->write(*data, size);
    this->bytesRead += size;
}

void User::sockReadInternalCopy(const QByteArray *data, const int size)
{
    /*
    ** TODO: use a CircularBuffer object to store the
    ** copied data and prevent useless ReAllocations
    */
    QByteArray copiedData;

    copiedData = QByteArray(data->data(), size);
    this->sockReadInternal(&copiedData, size);
}

void User::initUser()
{
    this->mc.devicesNames.output = this->objectName();
    this->manager->setUserConfig(this->mc);
    this->managerStarted = this->manager->start();
    if (!this->managerStarted)
    {
        say("failed to start manager");
        emit(sockClose(this));
        return;
    }
    this->initFlowChecker();
}

void User::initFlowChecker()
{
    //creating the flow checker (it check if the data are comming at the good speed)
    this->flowChecker = new FlowChecker(this->mc.format, this->checkInterval, this);
    connect(this->flowChecker, SIGNAL(ban(QString,int)), this, SLOT(ban(QString, int)));
    connect(this->flowChecker, SIGNAL(kick(QString)), this, SLOT(kill(QString)));
    connect(this->flowChecker, SIGNAL(debug(QString)), this, SLOT(say(QString)));
    this->flowChecker->start();
}

void User::appendToPendingBuffer(const QByteArray *data, const int size)
{
    /*
    ** the pending buffer is used to receive data from client while the User class
    ** has not been started yet (in case of threads),
    ** this buffer will be flushed into the manager when this->start(); will be called
    ** we NEED to make a copy of the data, not just using the pointer
    */
    this->pendingBuffer.append(QByteArray(data->data(), size));
}

void User::stop()
{
    QMutexLocker lock(this->mutex);
    if (this->inputDevice)
    {
        this->inputDevice->close();
        this->inputDevice->disconnect();
        delete(this->inputDevice);
        this->inputDevice = NULL;
    }
    lock.unlock();
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
        else this->mc.format->setSampleRate(*intVal);
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
        this->mc.format->setChannelCount(*intVal);
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
    /*
    ** this method is used to send data from the server
    ** to the client, in threaded mode this method does nothing in threaded udp
    ** because in udp mode the "socket" (in listen mode) is on a
    ** separate thread
    */
    QAbstractSocket *dev;

    if (!this->sock)  return;
    dev = (QAbstractSocket*) this->sock;
    if (dev->thread() != this->thread())
    {
        say("refusing to send: " + QString::fromLocal8Bit(data->data(), data->size()) + " : socket is on a separate thread");
    }
    else if (dev->isWritable())
    {
        dev->write(data->data(), data->size());
    }
}

void User::kill(const QString reason)
{
    QByteArray reason_b;
    QTcpSocket* sock;

    reason_b = QString("you where kicked: reason: " + reason).toLocal8Bit();
    this->flowChecker->setParent(NULL);
    say("kicking user: " + reason);
    send(&reason_b);
    if (this->sockType == ServerSocket::Tcp)
    {
        sock = (QTcpSocket*) this->sock;
        sock->close();
    }
    this->flowChecker->stop();
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
    this->security->addToBannedList(&host, banTime);
    kill(reason);
}

ServerSecurity* User::callSecurity()
{
    return this->security;
}

const QObject* User::getSocketPointer()
{
    return this->sock;
}

quint64 User::getBytesCount()
{
    return this->bytesRead;
}

QHostAddress User::getHostAddress()
{
    QHostAddress address;

    if (this->sockType == ServerSocket::Udp) address.setAddress(qobject_cast<QUdpSocket*>(this->sock)->peerAddress().toIPv4Address());
    else if (this->sockType == ServerSocket::Tcp) address.setAddress(qobject_cast<QTcpSocket*>(this->sock)->peerAddress().toIPv4Address());
    return address;
}

int User::getSpeed()
{
    int elaspedTime;
    int speed;

    //the speed will be returned in bytes per seconds
    this->lastBytesRead = this->bytesRead;
    elaspedTime = speedLastCheckTime.elapsed();
    if (!elaspedTime) return -1;

    speed = (this->bytesRead - this->lastBytesRead) / elaspedTime;

    this->speedLastCheckTime = QTime::currentTime();
    this->lastBytesRead = this->bytesRead;
    return speed;
}

Readini* User::getIni()
{
    return this->ini;
}

void User::moveToThread(QThread *thread)
{
    /*
    ** in case of a Tcp socket, we also move the socket to the user thread
    ** in case of udp socket: we can't because of the non connection aspect
    ** all user use the same udp listen socket
    */
    say("moving to an other thread");
    connect(thread, SIGNAL(finished()), this, SLOT(deleteLater()));
    QObject::moveToThread(thread);
    if (this->sockType == ServerSocket::Tcp)
    {
        this->sock->setParent(0);
        this->sock->moveToThread(thread);
        connect(thread, SIGNAL(finished()), this->sock, SLOT(deleteLater()));
    }
    this->isThreaded = true;
    say("moving done");
}

void User::showStats(void)
{
    say("readed : " + Size::getWsize(this->bytesRead));
}
