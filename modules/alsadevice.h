#ifdef ALSA

#ifndef ALSADEVICE_H
#define ALSADEVICE_H

#include <QIODevice>
#include <QString>
#include <alsa/asoundlib.h>
#include "audioformat.h"

class AlsaDevice : public QIODevice
{
    Q_OBJECT
public:
    explicit AlsaDevice(AudioFormat *format,QObject *parent = 0);
    ~AlsaDevice();
    bool open(OpenMode mode);
    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);
private:
    snd_pcm_t *playback_handle;
    snd_pcm_t *record_handle;
    snd_pcm_hw_params_t *hw_params;
    void say(const QString message);
    AudioFormat *format;
    bool getPcmFormat(_snd_pcm_format *pcmFormat);
    bool configureHandle(snd_pcm_t *handle);
signals:

public slots:

};

#endif // ALSADEVICE_H

#endif
