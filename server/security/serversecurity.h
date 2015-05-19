#ifndef SERVERSECURITY_H
#define SERVERSECURITY_H

#include <QObject>
#include <QList>
#include <QTime>
#include <QtNetwork/QHostAddress>
#include "readini.h"

class ServerSecurity : public QObject
{
    Q_OBJECT
public:
    explicit ServerSecurity(QObject *parent = 0);
    bool isAuthorisedHost(const QHostAddress *address);
    bool isBanned(const QHostAddress* address);
private:
    Readini* ini();
    QString serverConfig(const QString key);
    void say(const QString message);
    struct bannedUser {
        QHostAddress host;
        QTime bannedSince;
        int banTime;
    };
    QList<bannedUser> bannedHosts;
    void checkForUnban();
    bool checkForUnban(const int pos);

signals:
    void debug(const QString message);
public slots:
    void blockWithIpTables(const QHostAddress *address);
    void addToBannedList(const QHostAddress *address,const int banTime);
};

#endif // SERVERSECURITY_H
