#include "serversecurity.h"
#include "../servermain.h"
#include <QPair>
#include <QProcess>

//todo: maybe use iptables for linux to ban ?

ServerSecurity::ServerSecurity(QObject *parent) :
    QObject(parent)
{
}
Readini* ServerSecurity::ini() {
    return qobject_cast<ServerMain*>(this->parent())->getIni();
}
bool ServerSecurity::isAuthorisedHost(const QHostAddress *address) {
    //Reading the allowed hosts form the ini file, empty if not
    QString rawLine = serverConfig("allowed");

    //allowed host list
    QStringList hosts;

    //if there is ini value: assigning them to the allowed hosts
    if (!rawLine.isEmpty()) hosts = rawLine.split(" ");

    //if there is no hosts in the list: adding localhost and most common lan ip ranges
    if (hosts.isEmpty()) hosts << "127.0.0.1" << "192.168.0.0/24" << "192.168.1.0/24";

    QStringList::iterator i;
    for (i = hosts.begin() ; i != hosts.end() ; i++) {
        //parcouring the allowed host list and check if the current user is allowed, return true now if he is.
        if (address->isInSubnet(QHostAddress::parseSubnet(*i))) return true;
    }
    if (serverConfig("iptables").toInt()) blockWithIpTables(address);

    return isBanned(address);
}
QString ServerSecurity::serverConfig(const QString key) {
    return this->ini()->getValue("general",key);
}
void ServerSecurity::blockWithIpTables(const QHostAddress *address) {
#ifdef WIN32
    //no iptables on win32
    return;
#else

    const ServerSocket::type type = qobject_cast<ServerMain*>(this->parent())->getServerType();
    if (type == ServerSocket::Invalid) return;
    const QString typeString = ServerSocket::typeToString(type).toLower();

    QStringList args;
    args << "-A" << "INPUT" << "-p" << typeString << "-s" << address->toString() << "-j" << "DROP";
    QProcess p(this);
    const int result = p.execute("iptables",args);
    if (!result) say("blocked address with iptable: " + address->toString());
    else say("failed to block " + address->toString() + " with iptables: check perms");
#endif
}
void ServerSecurity::say(const QString message) {
    emit(debug("ServerSecurity: " + message));
}
void ServerSecurity::addToBannedList(const QHostAddress *address, const int banTime) {
    bannedUser user;
    user.bannedSince = QTime::currentTime();
    user.host = QHostAddress(*address);
    user.banTime = banTime;
    bannedHosts.append(user);
}
bool ServerSecurity::checkForUnban(const int pos) {
    const bannedUser user = bannedHosts.at(pos);
    if (user.bannedSince.elapsed() > user.banTime) {
        say("unbanning user: " + user.host.toString());
        bannedHosts.removeAt(pos);
        return true;
    }
    return false;
}

void ServerSecurity::checkForUnban() {
    int m =  bannedHosts.count();
    for (int i = 0 ; i < m ; i++) {
        if (checkForUnban(i)) {
            //because the size of bannedHost has changed, we decrement "m"
            m--;
            //and re-set the current position to last value to prevent item skip
            i--;
        }
    }
}
bool ServerSecurity::isBanned(const QHostAddress *address)  {
    int pos = bannedHosts.count() -1;
    while (pos--) {
        //if the user WAS banned and is not anymore: no need to check in the full list.
        if (checkForUnban(pos)) return false;

        if (bannedHosts.at(pos).host == *address) return true;
    }
    return false;
}
