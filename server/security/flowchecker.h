#ifndef FLOWCHECKER_H
#define FLOWCHECKER_H

#include <QObject>
#include <QTimer>
#include <QString>
#include "audioformat.h"

class FlowChecker : public QObject
{
    Q_OBJECT
public:
    explicit FlowChecker(AudioFormat* format,const int checkInterval,QObject *parent = 0);
    ~FlowChecker();
    bool start();
    int getInterval();
    void setFlowKick(const bool mode);
    void stop();
private:
    AudioFormat* format;
    QTimer timer;
    quint64 lastBytesRead;
    void say(const QString message);
    int warningCount;
    bool enableFlowKick;
signals:
    void debug(const QString message);
    void kick(const QString reason);
    void ban(const QString reason,const int duration);
private slots:
    void check();
public slots:

};

#endif // FLOWCHECKER_H
