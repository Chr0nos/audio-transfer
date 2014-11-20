#include "serversecurity.h"
#include "servermain.h"
#include <QPair>

ServerSecurity::ServerSecurity(QObject *parent) :
    QObject(parent)
{
}
Readini* ServerSecurity::ini() {
    return qobject_cast<ServerMain*>(this->parent())->getIni();
}
bool ServerSecurity::isAuthorisedHost(const QHostAddress *address) {
    QString rawLine = serverConfig("allowed");
    QStringList hosts;
    if (!rawLine.isEmpty()) hosts = rawLine.split(" ");
    if (hosts.isEmpty()) hosts << "127.0.0.1" << "192.168.0.0/24" << "192.168.1.0/24";

    QStringList::iterator i;
    for (i = hosts.begin() ; i != hosts.end() ; i++) {
        if (address->isInSubnet(QHostAddress::parseSubnet(*i))) return true;
    }

    return false;
}
QString ServerSecurity::serverConfig(const QString key) {
    return this->ini()->getValue("general",key);
}
