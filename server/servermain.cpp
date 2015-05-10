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
        AudioFormat *f = new AudioFormat();
        say("reading config file at: " + configFilePath);
        this->ini = new Readini(configFilePath);

        if (!ini->exists()) {
            say("no configuration file found.");
        }
        if (!ini->isSection("format"))
        {
            say("no [format] section in the configuration file, please update");
            f->setCodec("audio/pcm");
            f->setSampleRate(96000);
            f->setSampleSize(16);
            f->setChannelCount(2);

        #ifdef PULSE
                ini->setValue("general","output","pulse");
        #else
                ini->setValue("general","output","native");
        #endif
            say("using generic configuration for default audio format: 96khz, 16bits , 2 channels");
        }
        else
        {
            say("creating default audio format from configuration file...");
            f->setCodec(ini->getValue("format","codec"));
            f->setSampleRate(ini->getValue("format","sampleRate").toInt());
            f->setSampleSize(ini->getValue("format","sampleSize").toInt());
            f->setChannelCount(ini->getValue("format","channels").toInt());
            say("done.");
        }

        formatDefault = f;

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
    connect(this->srv,SIGNAL(readData(QHostAddress*,const quint16*,const QByteArray*,QUdpSocket*)),this,SLOT(readData(QHostAddress*,const quint16*,const QByteArray*,QUdpSocket*)));

    if (srv->startServer(type,port)) {
        say("server started on port " + QString::number(port));
        say("server mode: " + ServerSocket::typeToString(type));
        return true;
    }
    return false;
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
    User *newUser = new User(newSock,ServerSocket::Tcp,this->users);
    newSock->setObjectName(newSock->peerAddress().toString());
    say("adding new user: " + newSock->objectName());
    users->append(newUser);
}


void ServerMain::readData(QHostAddress *sender, const quint16 *senderPort, const QByteArray *data, QUdpSocket *udp) {
    /*
    ** TODO: connecter directement le sockread a l'user
    ** pour éviter alors de passer par ce parser qui ralentis trop le code
    */
    if (data->isEmpty()) return;
    User* user = NULL;
    int max;
    const int pos = users->indexOf(udp);

    (void) senderPort;
    if (pos < 0)
    {
        say("tryining to init new user: " + sender->toString());
        max = ini->getValue("general","maxUsers").toInt();
        if ((max) && (users->countUsers() >= max))
        {
            say("cannot add the new user: maximum user count reached");
            return;
        }
        if (!security->isAuthorisedHost(sender))
        {
            if (ini->getValue("general","showUdpRejected").toInt()) say("rejected data from: " + sender->toString());
            return;
        }
        say("adding udp user: " + sender->toString());
        user = new User(udp, ServerSocket::Udp, this->users);
        user->setObjectName(sender->toString());
        users->append(user);
    }
    else
    {
        /*
        ** here we disconnect the signal/slot fom the ServerSocket class to this one
        ** and re-connect the readdata signal to "User" class
        ** it's to avoid useless code here and mostly the UserHandler->indexOf
        ** for the value of "pos"
        */
        user = users->at(pos);
        disconnect(srv, SIGNAL(readData(QHostAddress*,const quint16*,const QByteArray*,QUdpSocket*)),
                   this, SLOT(readData(QHostAddress*,const quint16*,const QByteArray*,QUdpSocket*)));
        connect(srv, SIGNAL(readData(QHostAddress*,const quint16*,const QByteArray*,QUdpSocket*)),
                user, SLOT(readData(QHostAddress*,const quint16*,const QByteArray*,QUdpSocket*)));
    }
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
