#ifndef USER_H
#define USER_H

#include <QObject>
#include <QtNetwork/QTcpSocket>
#include "manager.h"
#include "audioformat.h"
#include "server/serversocket.h"
#include "modules/circulardevice.h"
#include "server/security/flowchecker.h"
#include "server/security/serversecurity.h"

class User : public QObject
{
    Q_OBJECT
public:
    explicit User(QObject* socket,ServerSocket::type type, QObject *parent = 0);
    ~User();
    void setFormat(const AudioFormat* format);
    QString getUserName();
    const QObject* getSocketPointer();
    void send(const QByteArray data);
    quint64 getBytesCount();
    QHostAddress getHostAddress();
    int getSpeed();
    ServerSecurity* callSecurity();
    Readini* getIni();
private:
    QObject* sock;
    Manager* manager;
    Manager::userConfig mc;
    quint64 bytesRead;
    bool readUserConfig(const QByteArray *data);
    ServerSocket::type sockType;
    QIODevice* inputDevice;
    qint64 connectionTime;
    QString peerAddress;
    bool managerStarted;
    void makeSpeedStatus();
    quint64 lastBytesRead;
    QTime speedLastCheckTime;
    FlowChecker* flowChecker;
    int checkInterval;
    void initUser();
    bool isPossibleConfigLine(const char* input, int lenght);
    void initFormat();
    bool readUserConfigOption(const QString *key, const QString *value, const int *intVal);

signals:
    void debug(const QString message);
    void sockClose(User* sender);
    void kicked();
private slots:
    void sockStateChanged(QAbstractSocket::SocketState state);
    void say(const QString message);
    void sockRead();
public slots:
    void sockRead(const QByteArray* data);
    void kill(const QString reason);
    void ban(const QString reason,const int banTime);
    void stop();
};

#endif // USER_H
