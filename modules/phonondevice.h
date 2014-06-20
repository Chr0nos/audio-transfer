#ifdef PHONON


#ifndef PHONONDEVICE_H
#define PHONONDEVICE_H

#include <QIODevice>

class PhononDevice : public QIODevice
{
    Q_OBJECT
public:
    explicit PhononDevice(QObject *parent = 0);
    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);
    bool open(OpenMode mode);

signals:

public slots:

};

#endif // PHONONDEVICE_H


#endif
