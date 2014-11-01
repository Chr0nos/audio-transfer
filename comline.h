#ifndef COMLINE_H
#define COMLINE_H

#include <QObject>
#include <QTextStream>
#include <QTimer>
#include "manager.h"
#include "readini.h"

class Comline : public QObject
{
    Q_OBJECT
public:
    explicit Comline(QStringList *argList, QObject *parent = 0);
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

signals:
    void quit();
public slots:
    void sockClose();
    void debug(QString message);

};

#endif // COMLINE_H
