#ifdef PULSE

#ifndef PULSE_H
#define PULSE_H

#include <pulse/simple.h>
#include <pulse/channelmap.h>

#include <QObject>
#include <QIODevice>
#include <QtMultimedia/QAudioFormat>
#include <QString>

class Pulse : public QObject {
    Q_OBJECT
public:
    explicit Pulse(const QString host, QAudioFormat format, QObject *parent = 0);
    ~Pulse();
    void write(const QByteArray &data);
    QIODevice* getDevice();
    bool makeChannelMap(pa_channel_map *map);
private:
    pa_simple *s;
    pa_sample_spec ss;
    QIODevice *device;
    QAudioFormat format;
    pa_sample_format getSampleSize();

signals:
    void say(const QString message);
public slots:

};

class PulseDevice : public QIODevice {
public:
    PulseDevice(pa_simple** pa, QObject* parent = 0);
    bool isOpen();
private:
    qint64 writeData(const char *data, qint64 len);
    qint64 readData(char *data, qint64 maxlen);
    pa_simple **pulseAudio;
    QObject *parent;
};

#endif // PULSE_H

#endif
