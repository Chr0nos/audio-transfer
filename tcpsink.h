#ifndef TCPSINK_H
#define TCPSINK_H

#include <QObject>
#include <QString>
#include <QTcpSocket>

class TcpSink : public QObject
{
    Q_OBJECT
public:
    explicit TcpSink(QObject *parent = 0);
    ~TcpSink();
    QIODevice* getDevice();
    void send(QString *message);
    void connectToHost(const QString targetAddress,const int targetPort);
    void disconnectFromHost();

private:
    int port;
    QString host;
    QTcpSocket *sock;
    QIODevice *devOut;

signals:
    void readyWrite();
    void connected();
    void disconnected();
    void reply(const QString line);

public slots:
private slots:
    void sockRead();

};

#endif // TCPSINK_H
