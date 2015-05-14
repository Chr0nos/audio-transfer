#ifndef COMLINE_H
#define COMLINE_H

#include <QObject>
#include <QTextStream>
#include <QTimer>
#include "manager.h"
#include "readini.h"
#ifdef SERVER
#include "server/servermain.h"
#endif

class Comline : public QObject
{
    Q_OBJECT
public:
    explicit Comline(QStringList *argList, QObject *parent = 0);
    ~Comline();
    bool start();
    static void circularTest();
public slots:
    void showStats();
private:
    Manager* manager;
    QTextStream* out;
    quint64 lastReadedValue;
    QTimer *timer;
    void parse(QStringList *argList);
    Manager::userConfig mc;
    Readini* ini;
    bool initConfig();
    bool initResult;
    void say(const QString message);
    bool bDebug;
    bool quiet;
    bool sayUseTime;
    QString iniPath;
    void loadIni();
    void showHelp();
    void writePid(QString *value);
    ServerSocket::type serverType;
#ifdef SERVER
    ServerMain* srv;
    void testDevice();
    void makeServer();
#endif
    void showCommonRates();
    void showQStringList(QStringList* list);

signals:
    void quit();
public slots:
    void sockClose();
    void debug(QString message);
    void debugTrigger();

};

#endif // COMLINE_H
