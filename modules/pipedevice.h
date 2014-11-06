#ifndef PIPEDEVICE_H
#define PIPEDEVICE_H

#include <QIODevice>

class PipeDevice : public QIODevice
{
    Q_OBJECT
public:
    explicit PipeDevice(QObject *parent = 0);

signals:

public slots:

};

#endif // PIPEDEVICE_H
