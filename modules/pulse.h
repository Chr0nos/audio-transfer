#ifdef PULSE

#ifndef PULSE_H
#define PULSE_H

#include <pulse/simple.h>
#include <pulse/channelmap.h>
#include <pulse/pulseaudio.h>

#include <QObject>
#include <QString>
#include <QStringList>
#include <QTime>
#include <QByteArray>
#include <QThread>
#include "circularbuffer.h"

#include "audioformat.h"
#include "modules/moduledevice.h"

/* because the pulse simple api is blocant
 * i needed to make it on a separate thread
 * it emit 'readyRead' when it's reading something
 */
class PulseDeviceRead : public QThread {
    Q_OBJECT
public:
    PulseDeviceRead(pa_simple* stream, CircularBuffer* buffer, QObject* parent);
    ~PulseDeviceRead();
    void run();
    QByteArray getAvailableData();
private:
    void readFromPaSimple();
    pa_simple* rec;
    void say(const QString message);
    CircularBuffer *readBuffer;
signals:
    void debug(const QString message);
    void readyRead();
};


class PulseDevice : public ModuleDevice {
    Q_OBJECT
public:
    PulseDevice(const QString name, const QString target, AudioFormat *format, QObject *parent);
    ~PulseDevice();
    bool open(OpenMode mode);
    void close();
    qint64 bytesAvailable();
    QStringList getDevicesNames(QIODevice::OpenMode mode);
    static ModuleDevice *factory(QString name, AudioFormat *format, void *userData, QObject *parent);

private:
    qint64 writeData(const char *data, qint64 len);
    qint64 readData(char *data, qint64 maxlen);
    QObject *parent;
    pa_simple *s;
    pa_simple *rec;
    pa_sample_spec ss;
    pa_sample_format getSampleSize();
    //bool makeChannelMap(pa_channel_map *map);
    quint64 getBiteRate();
    AudioFormat *format;
    QString target;
    QString name;
    quint64 lastWritePos;
    quint64 bytesWrite;
    quint64 bytesRead;
    QByteArray buffer;
    bool prepare(QIODevice::OpenMode mode,pa_simple **pulse);
    PulseDeviceRead *record;
    CircularBuffer *readBuffer;
private slots:
    void testSlot();
    void say(const QString message);
signals:
    void readyRead();
    void debug(const QString message);
};

#endif // PULSE_H

#endif
