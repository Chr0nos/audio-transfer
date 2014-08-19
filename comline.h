#ifndef COMLINE_H
#define COMLINE_H

#include <QObject>
#include <QTextStream>
#include <QTimer>
#include "manager.h"

class Comline : public QObject
{
    Q_OBJECT
public:
    explicit Comline(QString *args,QObject *parent = 0);
    bool start();
public slots:
    void showStats();
private:
    Manager* manager;
    QTextStream* out;
    quint64 lastReadedValue;
    QTimer *timer;

signals:

public slots:
    void debug(QString message);
    void sockClose();
};

#endif // COMLINE_H
