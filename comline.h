#ifndef COMLINE_H
#define COMLINE_H

#include <QObject>
#include "manager.h"

class Comline : public QObject
{
    Q_OBJECT
public:
    explicit Comline(QObject *parent = 0);
    void start();
private:
    Manager* manager;

signals:

public slots:

};

#endif // COMLINE_H
