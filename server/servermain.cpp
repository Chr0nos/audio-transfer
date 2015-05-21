#include "servermain.h"
#include "audioformat.h"
#include "server/serversocket.h"
#include "server/security/serversecurity.h"

//TODO: implement a CircularBuffer to take care about remote user buffer

ServerMain::ServerMain(const QString configFilePath, QObject *parent) :
    QObject(parent)
{
    this->srv = NULL;
    this->ini = NULL;
    this->users = new UserHandler(this);
    this->security = new ServerSecurity(this);
    connect(users,SIGNAL(debug(QString)),this,SIGNAL(debug(QString)));
    this->formatDefault = NULL;
    this->configFilePath = configFilePath;

}
ServerMain::~ServerMain() {
    if (ini) ini->deleteLater();
    if (formatDefault) delete(formatDefault);
    users->deleteLater();
    delete(formatDefault);
}

bool ServerMain::listen(ServerSocket::type type)
{
    if (!ini)
    {
        this->initFormat();
    }
    say("creating server socket");
    quint16 port = ini->getValue("general","port").toInt();

    if (!port) {
        say("using default port: 1042");
        port = 1042;
    }

    this->srv = new ServerSocket(this);
    connect(this->srv,SIGNAL(debug(QString)),this,SLOT(say(QString)));
    connect(this->srv,SIGNAL(sockOpen(QTcpSocket*)),this,SLOT(sockOpen(QTcpSocket*)));
    connect(this->srv,SIGNAL(readData(QHostAddress*,const QByteArray*,QUdpSocket*)),
            this,SLOT(readData(QHostAddress*,const QByteArray*,QUdpSocket*)));

    if (srv->startServer(type,port)) {
        say("server started on port " + QString::number(port));
        say("server mode: " + ServerSocket::typeToString(type));
        return true;
    }
    return false;
}

void ServerMain::initFormat()
{
    AudioFormat *f;

    f = new AudioFormat();
    say("reading config file at: " + configFilePath);
    this->ini = new Readini(configFilePath);

    if (!this->ini->exists()) {
        say("no configuration file found.");
    }
    if (!this->ini->isSection("format"))
    {
        say("no [format] section in the configuration file, please update");
        f->setCodec("audio/pcm");
        f->setSampleRate(96000);
        f->setSampleSize(16);
        f->setChannelCount(2);

    #ifdef PULSE
            this->ini->setValue("general", "output", "pulse");
    #else
            this->ini->setValue("general", "output", "native");
    #endif
        say("using generic configuration for default audio format: 96khz, 16bits , 2 channels");
    }
    else
    {
        say("creating default audio format from configuration file...");
        f->setCodec(this->ini->getValue("format","codec"));
        f->setSampleRate(this->ini->getValue("format","sampleRate").toInt());
        f->setSampleSize(this->ini->getValue("format","sampleSize").toInt());
        f->setChannelCount(this->ini->getValue("format","channels").toInt());
        say("done.");
    }
    formatDefault = f;
}

void ServerMain::say(const QString message)
{
    emit(debug("ServerMain: " + message));
}

void ServerMain::sockOpen(QTcpSocket *newSock) {
    const int max = ini->getValue("general","maxUsers").toInt();

    if ((max) && (users->countUsers() >= max))
    {
        say("maximum users count reach: " + QString::number(max) + " -> rejecting new user from: " + newSock->peerAddress().toString());
        newSock->deleteLater();
        return;
    }
    say("adding new user: " + newSock->objectName());
    users->createUser(newSock, ServerSocket::Tcp, newSock->peerAddress().toString());
}


void ServerMain::readData(QHostAddress *sender, const QByteArray *data, QUdpSocket *udp)
{
    /*
    ** this is the dispatched of udp readed data
    ** ALL udp client will receive the sound stream throuth this
    ** method, is perfectly possible that the client is sending
    ** an empty udp packet, so it's important to let the isEmpty
    ** verification at the first line to dont alocate memory for
    ** nothing
    **
    ** logic of the method:
    ** -> search for the user in the users list (stored in UserHandler)
    ** -> if the user is not found : we create one an assignat it's pointer to
    **    "user" (if allowed by the security)
    ** -> else we set the user pointer from the handler to "user"
    ** -> in both case we write the sount with user->write(data);
    */
    if (data->isEmpty()) return;
    User *user;
    int max;
    int pos;

    pos = this->users->indexOf(udp);
    if (pos < 0)
    {
        say("trying to init new user: " + sender->toString());
        max = this->ini->getValue("general","maxUsers").toInt();
        if ((max) && (users->countUsers() >= max))
        {
            say("cannot add the new user: maximum user count reached");
            return;
        }
        if ((!this->security->isAuthorisedHost(sender)) && (ini->getValue("general", "showUdpRejected").toInt()))
        {
            say("rejected data from: " + sender->toString());
            return;
        }
        say("adding udp user: " + sender->toString());
        user = this->users->createUser(udp, ServerSocket::Udp, sender->toString());
    }
    else user = users->at(pos);
    user->sockRead(data);
}

Readini* ServerMain::getIni()
{
    return ini;
}

ServerSocket::type ServerMain::getServerType()
{
    if (!this->srv) return ServerSocket::Invalid;
    return this->srv->getServerType();
}
