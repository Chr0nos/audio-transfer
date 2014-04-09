#ifdef PULSE

#include "pulse.h"

#include <pulse/simple.h>
#include <pulse/error.h>
#include <pulse/channelmap.h>

#include <QtMultimedia/QAudioFormat>
#include <QString>

#include <QDebug>

Pulse::Pulse(const QString target, QAudioFormat format, QObject *parent) :
    QObject(parent)
{
    emit(say("[pulse] : init : start"));
    //s is a pa_simple*
    s = 0;
    //device: QIODevice*
    device = 0;
    this->format = format;

    pa_sample_spec ss;
    ss.format = getSampleSize();
    ss.channels = format.channelCount();
    ss.rate = format.sampleRate();
    if ((quint32) ss.rate == (quint32) PA_SAMPLE_INVALID) {
        say("pulse: error: invalid sample rate");
        return;
    }

    pa_channel_map map;
    makeChannelMap(&map);

    int errorCode = 0;

    const char* serverData = target.toStdString().c_str();
    if (target.isEmpty()) serverData = NULL;
    qDebug() << "pulseaudio server: " + target;

    //PA_STREAM_PLAYBACK
    //PA_STREAM_RECORD

    s = pa_simple_new(serverData,
                      "Audio-Transfer-Client",          // Our application's name.
                      PA_STREAM_PLAYBACK,               // stream direction
                      NULL,                             // Use the default device.
                      "Audio-Transfer-Client",          // Description of our stream.
                      &ss,                              // Our sample format.
                      &map,                             // Use default channel map
                      NULL,                             // Use default buffering attributes.
                      &errorCode                        // error code pointer (int).
                      );
    if (!s) {
        qDebug() << "connection failed: " + QString::number(errorCode) << pa_strerror(errorCode);
        qDebug() << "arguments: " << target;
        say("unable to set a pulseaudio connexion:" + QString(pa_strerror(errorCode)));
        return;
    }
   device = new PulseDevice(&s,this);
   device->open(QIODevice::WriteOnly);
   qDebug() << "pulseaudio device: " << device;
   say("[pulse] : init : done");
}
Pulse::~Pulse() {
    if (s) {
        pa_simple_free(s);
    }
}
void Pulse::write(const QByteArray &data) {
    device->write(data);
}
QIODevice* Pulse::getDevice() {
    return device;
}
bool Pulse::makeChannelMap(pa_channel_map* map) {
    map->channels = format.channelCount();
    pa_channel_map_init_auto(map,format.channelCount(),PA_CHANNEL_MAP_ALSA);
    return true;
}
pa_sample_format Pulse::getSampleSize() {
    switch (format.sampleSize()) {
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

PulseDevice::PulseDevice(pa_simple **pa, QObject *parent) {
    this->pulseAudio = pa;
    this->parent = parent;
    emit(readyRead());
}
qint64 PulseDevice::writeData(const char *data, qint64 len) {
    pa_simple_write(*this->pulseAudio,data,len,NULL);
    pa_simple_flush(*this->pulseAudio,NULL);
    return len;
}
qint64 PulseDevice::readData(char *data, qint64 maxlen) {
    pa_simple_read(*this->pulseAudio,data,maxlen,NULL);
    return maxlen;
}
bool PulseDevice::isOpen() {
    if (*this->pulseAudio) return true;
    return false;
}

#endif
