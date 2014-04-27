#ifndef ZERODEVICE_H
#define ZERODEVICE_H

#include <QIODevice>
#include <QAudioFormat>
#include <QTimer>


class ZeroDevice : public QIODevice
{
    Q_OBJECT
public:
    explicit ZeroDevice(QAudioFormat *format, QObject *parent = 0);
    bool open(OpenMode mode);
private:
    qint64 writeData(const char *data, qint64 len);
    qint64 readData(char *data, qint64 maxlen);
    QTimer* timer;
    QAudioFormat* format;
    quint64 bytesCountPs;
    quint32 lastReadTime;
signals:
    void readyRead();
public slots:

};

#endif // ZERODEVICE_H
