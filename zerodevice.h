#ifndef ZERODEVICE_H
#define ZERODEVICE_H

#include <QIODevice>
#include <QTimer>

class ZeroDevice : public QIODevice
{
    Q_OBJECT
public:
    explicit ZeroDevice(QObject *parent = 0);
    bool open(OpenMode mode);
private:
    qint64 writeData(const char *data, qint64 len);
    qint64 readData(char *data, qint64 maxlen);
    QTimer* timer;
signals:

public slots:

};

#endif // ZERODEVICE_H
