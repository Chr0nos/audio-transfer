#ifdef PULSE

#include "pulse.h"

#include <pulse/simple.h>
#include <pulse/error.h>
#include <pulse/channelmap.h>

#include <QtMultimedia/QAudioFormat>
#include <QString>
#include <QTimer>

#include <QDebug>

bool PulseDevice::makeChannelMap(pa_channel_map* map) {
    if (format.channelCount() > PA_CHANNELS_MAX) {
        say("error: max channels is: " + QString::number(PA_CHANNELS_MAX) + " current is: " + QString::number(format.channelCount()));
        return false;
    }
    map->channels = format.channelCount();
    pa_channel_map_init_auto(map,format.channelCount(),PA_CHANNEL_MAP_ALSA);
    //pa_channel_map_init_auto(map,2,PA_CHANNEL_MAP_DEFAULT);
    return true;
}
pa_sample_format PulseDevice::getSampleSize() {
    switch (format.sampleSize()) {
        case 8:
            return PA_SAMPLE_U8;
        case 16:
            return PA_SAMPLE_S16NE;
        case 24:
            return PA_SAMPLE_S24NE;
        case 32:
            return PA_SAMPLE_S32NE;
        default:
            return PA_SAMPLE_INVALID;
    }
}
void PulseDevice::testSlot() {
    say("test slot called");
    emit(readyRead());
}

PulseDevice::PulseDevice(const QString name,const QString target, QAudioFormat format, QObject *parent) {
    this->parent = parent;
    this->debugMode = true;
    say("init : start");
    say("requested format: channels:" + QString::number(format.channelCount()) + " sampleRate:" + QString::number(format.sampleRate()) + " sampleSize:" + QString::number(format.sampleSize()));
    //s is a pa_simple*
    s = 0;
    rec = 0;
    this->format = format;
    this->target = target;
    this->name = name;
    this->latencyRec = -1;
    this->timer = new QTimer(this);
    this->timer->setInterval(100);
    connect(this->timer,SIGNAL(timeout()),this,SIGNAL(readyRead()));
    //connect(this->timer,SIGNAL(timeout()),this,SLOT(testSlot()));

    ss.format = getSampleSize();
    ss.channels = format.channelCount();
    ss.rate = format.sampleRate();
    if ((quint32) ss.rate == (quint32) PA_SAMPLE_INVALID) {
        say("pulse: error: invalid sample rate");
        return;
    }


    say("init : done");


    emit(readyRead());
}
PulseDevice::~PulseDevice() {
    say("destructor called");
    if (this->isOpen()) this->close();
    if (this->timer) this->timer->deleteLater();
}
void PulseDevice::close() {
    QIODevice::close();
    if (s) {
        say("closing playback");
        pa_simple_free(s);
        s = 0;
    }
    if (rec) {
        say("closing record");
        pa_simple_free(rec);
        rec = 0;
        latencyRec = -1;
    }
    timer->stop();
}

qint64 PulseDevice::writeData(const char *data, qint64 len) {
    if (!s) return -1;
    int error = 0;
    if (pa_simple_write(s,data,len,&error) < 0) {
        say("write error: " + QString(pa_strerror(error)));
        return -1;
    }
    else {
        //pa_simple_flush(s,NULL);
        pa_simple_drain(s,&error);
    }
    return len;
}
qint64 PulseDevice::readData(char *data, qint64 maxlen) {
    //qDebug() << "read, request: " << maxlen;
    if (!rec) return -1;
    if (!maxlen) return -1;
    int error = 0;
    const int bytesRead = pa_simple_read(rec,data,maxlen,&error);
    //qDebug() << "read: " << bytesRead;
    if (bytesRead < 0) {
        say("cannot read data: " + QString(pa_strerror(error)));
        return -1;
    }
    return bytesRead;
}
void PulseDevice::say(const QString message) {
    if (this->debugMode) qDebug() << "PULSE" << message;
}
bool PulseDevice::open(OpenMode mode) {
    QIODevice::open(mode);
    say("opening device");
    bool state = false;
    pa_channel_map map;
    if (!makeChannelMap(&map)) return false;
    int errorCode = 0;
    //server target
    const char* serverData = this->target.toStdString().c_str();
    if (this->target.isEmpty()) serverData = NULL;

    if ((mode == WriteOnly) || (mode == ReadWrite)) {
        say("create playback");

        s = pa_simple_new(serverData,
                          name.toStdString().data(),        // Our application's name.
                          PA_STREAM_PLAYBACK,               // stream direction
                          NULL,                             // Use the default device.
                          "Audio-Transfer-Server Playback",          // Description of our stream.
                          &ss,                              // Our sample format.
                          &map,                             // Use default channel map
                          NULL,                             // Use default buffering attributes.
                          &errorCode                        // error code pointer (int).
                          );
        if (!s) {
            qDebug() << "connection failed: " + QString::number(errorCode) << pa_strerror(errorCode);
            qDebug() << "arguments: " << target;
            say("connection playback failed: " + QString(pa_strerror(errorCode)));
            return false;
        }
        state = true;
    }
    if ((mode == ReadOnly) || (mode == ReadWrite)) {
        say("create record");
        errorCode = 0;
        rec = pa_simple_new(serverData,
                          name.toStdString().data(),        // Our application's name.
                          PA_STREAM_RECORD,               // stream direction
                          NULL,                             // Use the default device.
                          "Audio-Transfer-Server Record",          // Description of our stream.
                          &ss,                              // Our sample format.
                          &map,                             // Use default channel map
                          NULL,                             // Use default buffering attributes.
                          &errorCode                        // error code pointer (int).
                          );
        if (!rec) {
            say("connection record failed: " + QString(pa_strerror(errorCode)));
            return false;
        }
        latencyRec = (int) pa_simple_get_latency(rec,NULL);
        say("current record latency: " + QString::number(latencyRec) + "usecs");
        state = true;
        emit(readyRead());
        timer->start();
        say("record timer started");
    }
    if (state) say("open state: ok");
    else say("open state: failed");

    return state;
}
void PulseDevice::deleteLater() {
    delete(this);
    QIODevice::deleteLater();
}

#endif
