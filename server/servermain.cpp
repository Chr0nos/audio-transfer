#include "servermain.h"
#include "audioformat.h"
#include "server/serversocket.h"
#include "server/serversecurity.h"

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

bool ServerMain::listen(ServerSocket::type type) {
    if (!ini) {
        say("reading config file at: " + configFilePath);
        this->ini = new Readini(configFilePath);
        if (!ini->exists()) {
            say("no configuration file found.");
            exit(0);
        }
        else if (!ini->isSection("format")) {
            say("no [format] section in the configuration file, please update");
            exit(0);
        }

        AudioFormat *f = new AudioFormat();
        f->setCodec(ini->getValue("format","codec"));
        f->setSampleRate(ini->getValue("format","sampleRate").toInt());
        f->setSampleSize(ini->getValue("format","sampleSize").toInt());
        f->setChannelCount(ini->getValue("format","channels").toInt());
        formatDefault = f;

    }
    say("creating server socket");
    quint16 port = ini->getValue("general","port").toInt();
    port = 1043;
    if (!port) {
        say("error: invalid port specified: " + QString::number(port));
        exit(0);
    }

    this->srv = new ServerSocket(this);
    connect(this->srv,SIGNAL(debug(QString)),this,SLOT(say(QString)));
    connect(this->srv,SIGNAL(sockOpen(QTcpSocket*)),this,SLOT(sockOpen(QTcpSocket*)));
    connect(this->srv,SIGNAL(readData(QHostAddress*,const quint16*,const QByteArray*,QUdpSocket*)),this,SLOT(readData(QHostAddress*,const quint16*,const QByteArray*,QUdpSocket*)));

    if (srv->startServer(type,port)) {
        say("server started on port " + QString::number(port));
        return true;
    }
    return false;
}
void ServerMain::say(const QString message) {
    emit(debug("ServerMain: " + message));
}
void ServerMain::sockOpen(QTcpSocket *newSock) {
    User *newUser = new User(newSock,ServerSocket::Tcp,this);
    newSock->setObjectName(newSock->peerAddress().toString());
    say("adding new user: " + newSock->objectName());
    users->append(newUser);
}


void ServerMain::readData(QHostAddress *sender, const quint16 *senderPort, const QByteArray *data, QUdpSocket *udp) {
    if (data->isEmpty()) return;
    (void) senderPort;
    User* user = NULL;
    const int pos = users->indexOf(udp);
    if (pos < 0) {
        if (!security->isAuthorisedHost(sender)) {
            //say("rejected data from: " + sender->toString());
            return;
        }
        say("adding udp user: " + sender->toString());
        user = new User(udp,ServerSocket::Udp,this);
        user->setObjectName(sender->toString());

        users->append(user);
    }
    else {
        user = users->at(pos);
    }
    user->sockRead(data);
}
Readini* ServerMain::getIni() {
    return ini;
}
