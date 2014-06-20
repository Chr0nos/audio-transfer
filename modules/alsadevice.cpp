#ifdef ALSA

#include "alsadevice.h"
#include "audioformat.h"
#include <alsa/asoundlib.h>
#include <QMap>

#include <QDebug>

AlsaDevice::AlsaDevice(AudioFormat *format, QObject *parent) :
    QIODevice(parent)
{
    say("init");
    playback_handle = 0;
    record_handle = 0;
    hw_params = 0;
    this->format = format;
}
AlsaDevice::~AlsaDevice() {
    if (playback_handle) snd_pcm_close(playback_handle);
    say("destroyed");
}

bool AlsaDevice::open(OpenMode mode) {
    QIODevice::open(mode);
    bool state = false;
    if ((mode == QIODevice::WriteOnly) || (mode == QIODevice::ReadWrite)) {
        say("opening playback handle");
        if (this->configureHandle(playback_handle)) {
            say("opened");
            state = true;
        }
        else state = false;
    }
    else if ((mode == QIODevice::ReadOnly) || (mode == QIODevice::ReadWrite)) {
        say("opening capture handle");
        if (this->configureHandle(record_handle)) {
            say("opened");
            state = true;
        }
        else state = false;
    }
    return state;
}
qint64 AlsaDevice::readData(char *data, qint64 maxlen) {
    if (!record_handle) return -1;
    int err;
    if ((err = snd_pcm_readi(record_handle,data,maxlen) != maxlen)) say("read from audio audio interface failed: " + QString(snd_strerror(err)));
    return maxlen;
}
qint64 AlsaDevice::writeData(const char *data, qint64 len) {
    if (!playback_handle) return -1;
    int err;
    if ((err = snd_pcm_writei(playback_handle,data,len)) != len) say("write to audio interface failed: " + QString(snd_strerror(err)));
    return len;
}
void AlsaDevice::say(const QString message) {
    qDebug() << "ALSA: " + message;
}


bool AlsaDevice::getPcmFormat(_snd_pcm_format *pcmFormat) {
    QMap<int,_snd_pcm_format> samplesSizes;
    samplesSizes[8] = SND_PCM_FORMAT_S8;
    samplesSizes[16] = SND_PCM_FORMAT_S16_LE;
    samplesSizes[18] = SND_PCM_FORMAT_S18_3LE;
    samplesSizes[20] = SND_PCM_FORMAT_S20_3LE;
    samplesSizes[24] = SND_PCM_FORMAT_S24_3LE;
    samplesSizes[32] = SND_PCM_FORMAT_S32_LE;
    if (samplesSizes.contains(format->getSampleSize())) {
        *pcmFormat = samplesSizes[format->getSampleSize()];
        return true;
    }
    else {
        say("unsuported sampleSize: " + QString::number(format->getSampleSize()));
        return false;
    }
    return false;
}

bool AlsaDevice::configureHandle(snd_pcm_t *handle) {
    int err;
    QString device = "hw:0,0";
    unsigned int sampleRate = format->getSampleRate();
    //todo : replace this by a proper method (with a switch)
    _snd_pcm_format currentSampleSize;
    if (!this->getPcmFormat(&currentSampleSize)) {
        say("cannot use requested sample size.");
        return false;
    }


    if ((err = snd_pcm_open(&handle,device.toLocal8Bit(), SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        say("cannot open audio device: " + QString(snd_strerror(err)));
        return false;
    }
    if ((err = snd_pcm_hw_params_malloc(&hw_params)) < 0) {
        say("cannot allocate hardware parameter structure: " + QString(snd_strerror(err)));
        return false;
    }
    if ((err = snd_pcm_hw_params_any(handle, hw_params)) < 0) {
        say("cannot initialize hardware parameter structure: " + QString(snd_strerror(err)));
        return false;
    }
    if ((err = snd_pcm_hw_params_set_access(handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
        say("cannot set access type: " + QString(snd_strerror(err)));
        return false;
    }
    if ((err = snd_pcm_hw_params_set_format(handle, hw_params,currentSampleSize)) < 0) {
        say("cannot set sample format: " + QString(snd_strerror(err)));
        return false;
    }
    if ((err = snd_pcm_hw_params_set_rate_near(handle, hw_params,&sampleRate,0)) < 0) {
        say("cannot set sample rate: " + QString(snd_strerror(err)));
        return false;
    }

    if ((err = snd_pcm_hw_params_set_channels(handle, hw_params, 2)) < 0) {
        say("cannot set channel count: " + QString(snd_strerror(err)));
        return false;
    }
    if ((err = snd_pcm_hw_params(handle, hw_params)) < 0) {
        say("cannot set parameters: " + QString(snd_strerror(err)));
        return false;
    }
    snd_pcm_hw_params_free (hw_params);
    return false;
}

#endif
