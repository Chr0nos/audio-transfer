#ifndef USER_H
#define USER_H

#include <QObject>
#include <QtNetwork/QTcpSocket>
#include <QMutex>
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
    void send(QByteArray *data);
    quint64 getBytesCount();
    QHostAddress getHostAddress();
    int getSpeed();
    ServerSecurity* callSecurity();
    Readini* getIni();
    void moveToThread(QThread *thread);
private:
    bool readUserConfig(const QByteArray *data);
    bool isPossibleConfigLine(const char* input, int lenght);
    void initUser();
    void initFormat();
    bool readUserConfigOption(const QString *key, const QString *value, const int *intVal);
    void makeSpeedStatus();
    void initModule();
    void sendSpecs();
    void initFlowChecker();
    QObject* sock;
    Manager* manager;
    Manager::userConfig mc;
    quint64 bytesRead;
    ServerSocket::type sockType;
    QIODevice* inputDevice;
    qint64 connectionTime;
    QString peerAddress;
    bool managerStarted;
    quint64 lastBytesRead;
    QTime speedLastCheckTime;
    FlowChecker* flowChecker;
    int checkInterval;
    Readini *ini;
    ServerSecurity *security;
    ServerSocket::type type;
    bool allowUserConfig;
    QString moduleName;
    QMutex *mutex;

signals:
    void debug(const QString message);
    void sockClose(User* sender);
    void kicked();
private slots:
    void sockStateChanged(QAbstractSocket::SocketState state);
    void say(const QString message);
    void sockRead();
public slots:
    void sockRead(const QByteArray *data);
    void readData(QHostAddress *sender, const quint16 *senderPort, const QByteArray *data, QUdpSocket *udp);
    void kill(const QString reason);
    void ban(const QString reason,const int banTime);
    void stop();
    void start();
};

#endif // USER_H
