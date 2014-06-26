#ifdef PULSE

#ifndef PULSE_H
#define PULSE_H

#include <pulse/simple.h>
#include <pulse/channelmap.h>
#include <pulse/pulseaudio.h>

#include <QObject>
#include <QIODevice>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QTime>
#include <QByteArray>

#include "audioformat.h"

class PulseDevice : public QIODevice {
    Q_OBJECT
public:
    PulseDevice(const QString name, const QString target, AudioFormat *format, QObject *parent);
    ~PulseDevice();
    bool open(OpenMode mode);
    void close();
    QStringList getDevicesNames(QIODevice::OpenMode mode);
private:
    qint64 writeData(const char *data, qint64 len);
    qint64 readData(char *data, qint64 maxlen);
    QObject *parent;
    pa_simple *s;
    pa_simple *rec;
    pa_sample_spec ss;
    pa_sample_format getSampleSize();
    bool makeChannelMap(pa_channel_map *map);
    quint64 getBiteRate();
    AudioFormat *format;
    void say(const QString message);
    QString target;
    QString name;
    bool debugMode;
    QTimer* timer;
    int latencyRec;
    quint64 lastWritePos;
    quint64 bytesWrite;
    quint64 bytesRead;
    QByteArray buffer;
    bool prepare(QIODevice::OpenMode mode,pa_simple **pulse);
private slots:
    void testSlot();
signals:
    void readyRead();
};

#endif // PULSE_H

#endif
