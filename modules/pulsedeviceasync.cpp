#ifdef PULSE

#include "pulsedeviceasync.h"
#include "audioformat.h"

#include <pulse/pulseaudio.h>
#include <pulse/error.h>

#include <QDebug>

/* /!\ WARNING /!\
 *  THIS MODULE STILL NOT WORKS PROPERLY, DONT USE IT PLEASE
 * /UNLESSE YOU WAN TO HELP ME TO MAKE IT USABLE...
 * */

PulseDeviceASync::PulseDeviceASync(AudioFormat *format, QObject *parent) :
    QIODevice(parent)
{
    say("init start");
    this->name = "Audio-Transfer-Client";
    this->format = format;
    //intialisating pointers to NULL value
    mainloop = NULL;
    mainloop_api = NULL;
    streamPlayback = NULL;
    streamRecord = NULL;
    context = NULL;

    say("setting sample specs");
    ss.channels = format->getChannelsCount();

    say("setting format");
    ss.format = this->getSampleFormat();

    say("setting sample rate");
    ss.rate = format->getSampleRate();

    say("init ok");
}

//cette fonction est crade, mais je n'ai pas le choix et je commence à en avoir mare de me faire téraformer le trous du cul par l'api bordelique de pulseaudio !
//futur moi: pardone moi.
void getSourcesDevices_cb(pa_context *c,const pa_source_info *i,int eol,void *userdata) {
    if (!c) return;
    QList<pa_source_info> *sources = (QList<pa_source_info>*) userdata;
    sources->append(*i);
}
void getSinkDevices_cb(pa_context *c,const pa_sink_info* i,int eol,void *userdata) {
    if (!c) return;
    QList<pa_sink_info> *sinks = (QList<pa_sink_info>*) userdata;
    sinks->append(*i);
}

QStringList PulseDeviceASync::getDevicesNames(QIODevice::OpenMode mode) {
    QStringList devicesNames;
    if (!context) return devicesNames;
    else if (mode == QIODevice::WriteOnly) {
        QList<pa_sink_info> sinks = getSinkDevices();
        const int m = sinks.count();
        for (int p = 0;p < m;p++) {
            devicesNames << sinks.at(p).name;
        }
    }
    else if (mode == QIODevice::ReadOnly) {
        QList<pa_source_info> sources = getSourcesDevices();
        const int m = sources.count();
        for (int p = 0;p < m;p++) devicesNames << sources.at(p).name;
    }
    return devicesNames;
}
QList<pa_source_info> PulseDeviceASync::getSourcesDevices() {
    QList<pa_source_info> sources;
    if (!context) return sources;
    pa_context_get_source_info_list(context,getSourcesDevices_cb,&sources);
    return sources;
}
QList<pa_sink_info> PulseDeviceASync::getSinkDevices() {
    QList<pa_sink_info> sinks;
    if (!context) return sinks;
    pa_context_get_sink_info_list(context,getSinkDevices_cb,&sinks);
    return sinks;
}

//ne surtout pas relire cette fonction: elle initialise tout un merdier pensé par les codeurs tordus de la lib de pulseaudio, *Dont touch: it's magic
//TGCM / *i have no idea of what i'm doing....
bool PulseDeviceASync::makeContext() {
    mainloop = pa_mainloop_new();

    if (!mainloop) return false;
    say("ok, got a main loop");

    mainloop_api = pa_mainloop_get_api(mainloop);
    if (!mainloop_api) return false;
    say("got an main loop api");

    //context = pa_context_new_with_proplist(mainloop_api,this->name.toStdString().c_str(),NULL);
    context = pa_context_new(mainloop_api,name.toUtf8().constData());

    say("context ok");
    qDebug() << "context" << context;
    return true;
}
bool PulseDeviceASync::open(OpenMode mode) {
    say("opening context/stream");

    if (!this->isValidSampleSepcs()) {
        say("invalid sample format: aborting");
        return false;
    }

    if (!context) {
        say("making context");
        if (!makeContext()) {
            say("failed to create context");
            return false;
        }
        say("context created.");
    }

    say("context ok");
    if (mode == QIODevice::WriteOnly) {

        if (!makeChannelMap(&map)) {
            say("unable to get a channel map");
            return false;
        }

        streamPlayback = pa_stream_new(context,name.toUtf8().constData(),
                      &ss,
                      &map);
        if (!streamPlayback) {
            say("unable to get a stream playback, the most common cause is that you ask a too high sample size, try with a lower (like 16)");
            return false;
        }
        say("ok got a stream playback");

        say("connecting playback");
        pa_stream_flags_t t;
        const int error = pa_stream_connect_playback(streamPlayback,  //stream pointer (pa_stream*)
                                   NULL,            //name of the sink (NULL to default)
                                   NULL,            //Buffering attributes, or NULL for default
                                   t,               //flags	Additional flags, or 0 for default
                                   NULL,            //volume Initial volume, or NULL for default
                                   NULL);           //sync_stream Synchronize this stream with the specified one, or NULL for a standalone stream
        //here i got the -15 -> invalid argument.
        say("return state: " + QString(pa_strerror(error)));
        if (error < 0) return false;
    }
    else if (mode == QIODevice::ReadOnly) {
        //todo
    }
    //qDebug() << "devices names: " << getDevicesNames(QIODevice::WriteOnly);
    QIODevice::open(mode);
    qDebug() << this->getDevicesNames(QIODevice::ReadOnly) << this->getDevicesNames(QIODevice::WriteOnly);
    return true;
}
bool PulseDeviceASync::isValidSampleSepcs() {
    if (ss.format == PA_SAMPLE_INVALID) return false;
    if (ss.channels == 0) return false;
    if (ss.rate == 0) return false;
    return true;
}

qint64 PulseDeviceASync::readData(char *data, qint64 maxlen) {
    if (!maxlen) return 0;
    //juste le temps d'implémenter la fonction de lecture pour éviter de se choper un warming.
    memset(data,0,1);
    return -1;
}
qint64 PulseDeviceASync::writeData(const char *data, qint64 len) {
    if (!len) return 0;
    if (!streamPlayback) return -1;
    pa_stream_write(streamPlayback,
                    data,
                    len,
                    NULL,
                    0,
                    PA_SEEK_ABSOLUTE
                    );
    //pa_stream_write()
    return -1;
}
void PulseDeviceASync::close() {
    context = NULL;
    say("closed");
}
void PulseDeviceASync::say(const QString message) {
    qDebug() << "PULSE ASYNC: " + message;
}
bool PulseDeviceASync::makeChannelMap(pa_channel_map* map) {
    if ((uint) format->getChannelsCount() > PA_CHANNELS_MAX) {
        say("error: max channels is: " + QString::number(PA_CHANNELS_MAX) + " current is: " + QString::number(format->getChannelsCount()));
        return false;
    }
    map->channels = format->getChannelsCount();
    pa_channel_map_init_auto(map,format->getChannelsCount(),PA_CHANNEL_MAP_ALSA);
    return true;
}
pa_sample_format PulseDeviceASync::getSampleFormat() {
    say("attemping to determinate format");
    if (!format) return PA_SAMPLE_INVALID;
    say("given sample size: " + QString::number(format->getSampleSize()));
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
QList<int> PulseDeviceASync::getValidSamplesSize() {
    QList<int> rates;
    rates << 8 << 16 << 24 << 32;
    return rates;
}
QList<int> PulseDeviceASync::getDefaultSamplesRates() {
    QList<int> rates;
    rates << 5512  << 8000 << 11025 << 16000 << 22050 << 32000 << 44100
          << 48000 << 64000 << 88200 << 96000 << 176400 << 192000 << 384000;
    return rates;
}

#endif
