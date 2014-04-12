#ifndef ZERODEVICE_H
#define ZERODEVICE_H

#include <QIODevice>

class ZeroDevice : public QIODevice
{
    Q_OBJECT
public:
    explicit ZeroDevice(QObject *parent = 0);

signals:

public slots:

};

#endif // ZERODEVICE_H
