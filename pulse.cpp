#ifdef PULSE

#include "pulse.h"
#include <pulse/simple.h>
#include <pulse/error.h>
#include <QtMultimedia/QAudioFormat>
#include <QString>
#include <QDebug>

Pulse::Pulse(const QString target, QAudioFormat format, QObject *parent) :
    QObject(parent)
{
    //s is a pa_simple*
    s = 0;
    //device: QIODevice*
    device = 0;

    pa_sample_spec ss;
    ss.format = PA_SAMPLE_S16NE;
    ss.channels = format.channelCount();
    ss.rate = format.sampleRate();
    int errorCode = 0;

    const char* serverData = target.toStdString().c_str();
    if (target.isEmpty()) serverData = NULL;
    qDebug() << "pulseaudio server: " + target;
    s = pa_simple_new(serverData,
                      "Audio-Transfer-Client",          // Our application's name.
                      PA_STREAM_PLAYBACK,
                      NULL,                             // Use the default device.
                      "Audio-Transfer-Client",          // Description of our stream.
                      &ss,                              // Our sample format.
                      NULL,                             // Use default channel map
                      NULL,                             // Use default buffering attributes.
                      &errorCode                        // error code pointer (int).
                      );
    if (!s) {
        qDebug() << "connection failed: " + QString::number(errorCode) << pa_strerror(errorCode);
        qDebug() << "arguments: " << target;
        error("unable to set a pulseaudio connexion:" + QString(pa_strerror(errorCode)));
        return;
    }
   device = new PulseDevice(&s,this);
   device->open(QIODevice::WriteOnly);
   qDebug() << "pulseaudio device: " << device;
}
Pulse::~Pulse() {
    if (s) {
        pa_simple_free(s);
        device->deleteLater();
    }
}

void Pulse::write(const QByteArray &data) {
    device->write(data);
}
QIODevice* Pulse::getDevice() {
    return device;
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
