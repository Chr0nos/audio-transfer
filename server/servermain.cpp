#include "servermain.h"
#include "audioformat.h"
#include "server/serversocket.h"
#include "server/security/serversecurity.h"

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
        AudioFormat *f = new AudioFormat();
        say("reading config file at: " + configFilePath);
        this->ini = new Readini(configFilePath);

        if (!ini->exists()) {
            say("no configuration file found.");
        }
        if (!ini->isSection("format")) {
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
        else {
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
