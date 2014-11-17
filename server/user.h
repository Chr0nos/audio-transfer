#ifndef USER_H
#define USER_H

#include <QObject>
#include <QtNetwork/QTcpSocket>
#include "manager.h"
#include "audioformat.h"
#include "server/serversocket.h"
#include "server/afkkiller.h"
#include "modules/circulardevice.h"

class User : public QObject
{
    Q_OBJECT
public:
    explicit User(QObject* socket,ServerSocket::type type, QObject *parent = 0);
    ~User();
    void setFormat(const AudioFormat* format);
    QString getUserName();
    void kill(const QString reason);
    const QObject* getSocketPointer();
    void send(const QByteArray data);
    quint64 getBytesCount();
private:
    QObject* sock;
    Manager* manager;
    Manager::userConfig mc;
    quint64 bytesRead;
    bool readUserConfig(const QByteArray *data);
    ServerSocket::type sockType;
    QIODevice* inputDevice;
    AfkKiller *afk;
    qint64 connectionTime;

signals:
    void debug(const QString message);
    void sockClose(User* sender);
private slots:
    void sockStateChanged(QAbstractSocket::SocketState state);
    void say(const QString message);
    void sockRead();
public slots:
    void sockRead(const QByteArray* data);

};

#endif // USER_H
