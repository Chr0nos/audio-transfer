#ifndef COMLINE_H
#define COMLINE_H

#include <QObject>
#include <QTextStream>
#include "manager.h"

class Comline : public QObject
{
    Q_OBJECT
public:
    explicit Comline(QString *args,QObject *parent = 0);
    void start();
private:
    Manager* manager;
    QTextStream* out;

signals:

public slots:
    void debug(QString message);
};

#endif // COMLINE_H
