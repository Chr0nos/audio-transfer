#ifndef SERVERSECURITY_H
#define SERVERSECURITY_H

#include <QObject>
#include <QList>
#include <QtNetwork/QHostAddress>
#include "readini.h"

class ServerSecurity : public QObject
{
    Q_OBJECT
public:
    explicit ServerSecurity(QObject *parent = 0);
    bool isAuthorisedHost(const QHostAddress *address);
private:
    Readini* ini();
    QString serverConfig(const QString key);
signals:

public slots:

};

#endif // SERVERSECURITY_H
