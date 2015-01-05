#ifdef PULSE

#include "pulse.h"

#include <pulse/simple.h>
#include <pulse/error.h>
#include <pulse/channelmap.h>

#include <QString>

#include <QDebug>

/*
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
*/

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
    this->lastWritePos = 0;
    record = NULL;
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
    if (s) pa_simple_free(s);
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
        if (record) {
            record->terminate();
            record->disconnect();
            delete(record);
            record = NULL;
        }
        pa_simple_free(rec);
        rec = NULL;
    }
}

qint64 PulseDevice::writeData(const char *data, qint64 len) {
    if (!s) return -1;
    if (!len) return 0;
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
    //qDebug() << "read, request: " << maxlen << bytesAvailable();
    if (!rec) {
        say("read requested but record stream is not open");
        return -1;
    }
    if (!maxlen) return 0;
    const int available = bytesAvailable();
    if (maxlen > available) {
        maxlen = available;
    }
    QByteArray sound = readBuffer->getCurrentPosData(maxlen);
    memcpy(data,sound.data(),maxlen);

    return maxlen;
}
void PulseDevice::say(const QString message) {
    emit(debug("Pulse: " + message));
}
bool PulseDevice::open(OpenMode mode) {
    QIODevice::open(mode);
    say("opening device");

    if (mode == QIODevice::ReadOnly) {
        say("creating read buffer");
        readBuffer = new CircularBuffer(2097152,"pulse module simple read buffer",this);
        connect(readBuffer,SIGNAL(readyRead(int)),this,SIGNAL(readyRead()));
        connect(readBuffer,SIGNAL(debug(QString)),this,SLOT(say(QString)));
        say("buffer is ready");

        return this->prepare(mode,&rec);
    }
    else if (mode == QIODevice::WriteOnly) return this->prepare(mode,&s);
    else if (mode == QIODevice::ReadWrite) {
        //if ((this->prepare(mode,&rec)) && (this->prepare(mode,&s))) return true;
        say("unsuported mode: readWrite");
        return false;
    }
    return false;
}
quint64 PulseDevice::getBiteRate() {
    return format->getBitrate();
}
bool PulseDevice::prepare(OpenMode mode, pa_simple **pulse) {
    QString name = "Audio-Transfer";
    char* serverHost = (char*) this->target.toStdString().c_str();
    //const char* serverHost = "127.0.0.1";
    if (this->target.isEmpty()) serverHost = NULL; //remove this line and you will make nightmares... (seriously!)

    /*
    pa_channel_map map;
    if (!makeChannelMap(&map)) {
        say("unable to make channels map");
        return false;
    }
    */

    pa_stream_direction direction;

    if (mode == QIODevice::WriteOnly) {
        say("creating playback");
        name.append(" playback");
        direction = PA_STREAM_PLAYBACK;
    }
    else if (mode == QIODevice::ReadOnly) {
        say("creating record");
        name.append(" record");
        direction = PA_STREAM_RECORD;
    }
    else {
        say("unsuported mod for prepare: readWrite.");
        return false;
    }

    int errorCode = 0;
    *pulse = pa_simple_new(serverHost,                  // target server
                           this->objectName().toLocal8Bit().data(), // Our application's name.
                           direction,                   // stream direction
                           NULL,                        // Use the default device.
                           name.toStdString().c_str(),  // Description of our stream.
                           &ss,                         // Our sample format. (&map for map)
                           NULL,                        // Use default channel map
                           NULL,                        // Use default buffering attributes.
                           &errorCode);                 // error code pointer (int).
    if (!*pulse) {
        say("error: cannot create stream: " + QString(pa_strerror(errorCode)));
        say("channels: " + QString::number(ss.channels));
        say("samplerate " + QString::number(ss.rate));
        say("target: " + QString(serverHost));
        say("error code: " + QString::number(errorCode));
        return false;
    }
    say("stream is ready");

    int latency = (int) pa_simple_get_latency(*pulse,NULL);
    say("current latency: " + QString::number(latency) + "usecs");

    if ((mode == QIODevice::ReadOnly) || (mode == QIODevice::ReadWrite)) {
        record = new PulseDeviceRead(*pulse,readBuffer,this);
        connect(record,SIGNAL(readyRead()),this,SIGNAL(readyRead()));
        connect(record,SIGNAL(debug(QString)),this,SLOT(say(QString)));
        say("starting record.");
        record->start();
        say("record started");
    }
    say("open ok");
    return true;
}


qint64 PulseDevice::bytesAvailable() {
    return QIODevice::bytesAvailable() + readBuffer->getAvailableBytesCount();
}


PulseDeviceRead::PulseDeviceRead(pa_simple *stream, CircularBuffer *buffer, QObject *parent) {
    this->setParent(parent);
    this->rec = stream;
    this->readBuffer = buffer;
}

void PulseDeviceRead::run() {
    say("starting");
    readFromPaSimple();
}
void PulseDeviceRead::readFromPaSimple() {
    if (!rec) {
        say("error: no stream set.");
        return;
    }

    u_int16_t data[4096]; //4Ko
    int bytesRead;

    int error = 0;
    while (true) {
        //here... okay, DONT ASK ME WHY ! but, it's apears: pa_simple_read will bloc until the buffer was fully filed and will return 0 (dont ask my why zero, it's stupid !)
        bytesRead = pa_simple_read(rec,data,sizeof(data),&error);
        if (error) {
            say("error: " + QString::number(error));
            return;
        }
        else if (bytesRead < 0) {
            say("cannot read data: " + QString(pa_strerror(error)));
        }

        else {
            //if the read was successfull
            if (!readBuffer->append((char*) data,sizeof(data))) say("unable to append new data to buffer.");
            //the parent class will be notified by the readBuffer "readyRead" signal emited after this append.
        }
    }
}
void PulseDeviceRead::say(const QString message) {
    emit(debug("PulseDeviceRead: " + message));
}

#endif
