#ifdef PULSE

#include "pulse.h"

#include <pulse/simple.h>
#include <pulse/error.h>
#include <pulse/channelmap.h>

#include <QString>
#include <QTimer>

#include <QDebug>

bool PulseDevice::makeChannelMap(pa_channel_map* map) {
    if ((uint) format->getChannelsCount() > PA_CHANNELS_MAX) {
        say("error: max channels is: " + QString::number(PA_CHANNELS_MAX) + " current is: " + QString::number(format->getChannelsCount()));
        return false;
    }
    map->channels = format->getChannelsCount();
    pa_channel_map_init_auto(map,format->getChannelsCount(),PA_CHANNEL_MAP_ALSA);
    //pa_channel_map_init_auto(map,2,PA_CHANNEL_MAP_DEFAULT);
    return true;
}
pa_sample_format PulseDevice::getSampleSize() {
    switch (format->getSampleSize()) {
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

PulseDevice::PulseDevice(const QString name, const QString target, AudioFormat *format, QObject *parent) {
    this->parent = parent;
    say("init : start");
    say("requested format: channels:" + QString::number(format->getChannelsCount()) + " sampleRate:" + QString::number(format->getSampleRate()) + " sampleSize:" + QString::number(format->getSampleSize()));
    //s is a pa_simple*
    s = NULL;
    rec = NULL;
    this->setObjectName("Audio-Transfer");
    this->format = format;
    this->target = target;
    this->name = name;
    this->latencyRec = -1;
    this->timer = new QTimer(this);
    this->timer->setInterval(500);
    this->timer->setObjectName("PulseAudio module timer");
    this->lastWritePos = 0;
    connect(this->timer,SIGNAL(timeout()),this,SIGNAL(readyRead()));
    //connect(this->timer,SIGNAL(timeout()),this,SLOT(testSlot()));

    ss.format = getSampleSize();
    ss.channels = format->getChannelsCount();
    ss.rate = format->getSampleRate();
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
    emit(aboutToClose());
    QIODevice::close();
    if (s) {
        say("closing playback");
        //deleting the unplayed buffer
        pa_simple_flush(s,NULL);
        pa_simple_free(s);
        s = NULL;
    }
    if (rec) {
        say("closing record");
        pa_simple_free(rec);
        rec = NULL;
        latencyRec = -1;
    }
    timer->stop();
}

qint64 PulseDevice::writeData(const char *data, qint64 len) {
    if (!s) return -1;
    int error = 0;
    bytesWrite += len;

    //here i play the sound
    if (pa_simple_write(s,data,len,&error) < 0) {
        say("write error: " + QString(pa_strerror(error)));
        return -1;
    }
    return len;
}
qint64 PulseDevice::readData(char *data, qint64 maxlen) {
    //qDebug() << "read, request: " << maxlen;
    if (!rec) {
        say("read requested but record stream is not open");
        return -1;
    }
    if (!maxlen) return 0;
    int error = 0;
    const int bytesRead = pa_simple_read(rec,data,maxlen,&error);
    //qDebug() << "read: " << bytesRead << pa_strerror(error) << "max: " << maxlen;

    if (bytesRead < 0) {
        say("cannot read data: " + QString(pa_strerror(error)));
        return -1;
    }
    return bytesRead;
}
void PulseDevice::say(const QString message) {
#ifdef DEBUG
    qDebug() << "PULSE" << message;
#endif
    emit(debug("Pulse: " + message));

}
bool PulseDevice::open(OpenMode mode) {
    QIODevice::open(mode);
    say("opening device");

    if (mode == QIODevice::ReadOnly) return this->prepare(mode,&rec);
    else if (mode == QIODevice::WriteOnly) return this->prepare(mode,&s);
    else if (mode == QIODevice::ReadWrite) {
        if ((this->prepare(mode,&rec)) && (this->prepare(mode,&s))) return true;
    }
    return false;
}
quint64 PulseDevice::getBiteRate() {
    return format->getBitrate();
}
bool PulseDevice::prepare(OpenMode mode, pa_simple **pulse) {
    QString name = "Audio-Trasnfer-Client";
    const char* serverHost = this->target.toStdString().c_str();
    if (this->target.isEmpty()) serverHost = NULL; //remove this line and you will make nightmares... (seriously!)

    pa_channel_map map;
    if (!makeChannelMap(&map)) {
        say("unable to make channels map");
        return false;
    }

    pa_stream_direction *direction;
    direction = new pa_stream_direction;

    if (mode == QIODevice::WriteOnly) {
        say("creating playback");
        name.append(" playback");
        *direction = PA_STREAM_PLAYBACK;
    }
    else if (mode == QIODevice::ReadOnly) {
        say("creating record");
        name.append(" record");
        *direction = PA_STREAM_RECORD;
    }

    int *errorCode = new int;
    *errorCode = 0;
    *pulse = pa_simple_new(serverHost,                  // target server
                           this->objectName().toLocal8Bit().data(),
                           // Our application's name.
                           *direction,                   // stream direction
                           NULL,                        // Use the default device.
                           name.toStdString().c_str(),  // Description of our stream.
                           &ss,                         // Our sample format.
                           &map,                        // Use default channel map
                           NULL,                        // Use default buffering attributes.
                           errorCode);                 // error code pointer (int).
    if (!*pulse) {
        say("error: cannot create stream: " + QString(pa_strerror(*errorCode)));
        say("channels: " + QString::number(ss.channels));
        say("samplerate " + QString::number(ss.rate));
        say("target: " + QString(serverHost));
        say("error code: " + QString::number(*errorCode));
        delete(errorCode);
        return false;
    }

    int latency = (int) pa_simple_get_latency(*pulse,NULL);
    say("current latency: " + QString::number(latency) + "usecs");

    if (mode == QIODevice::ReadOnly) {
        say("starting record timer.");
        timer->start();
    }
    say("open ok");
    delete(errorCode);
    delete(direction);
    return true;
}
qint64 PulseDevice::bytesAvailable() {
    //Todo: find how to get the good size
    return QIODevice::bytesAvailable();
}

#endif
