#ifndef VOLUMECHANGE_H
#define VOLUMECHANGE_H

#include <QIODevice>

class VolumeChange : public QIODevice
{
    Q_OBJECT
public:
    explicit VolumeChange(QObject *parent = 0);

signals:

public slots:

};

#endif // VOLUMECHANGE_H
