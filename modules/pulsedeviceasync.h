#ifdef PULSE

#ifndef PULSEDEVICEASYNC_H
#define PULSEDEVICEASYNC_H

#include <QIODevice>
#include <QStringList>
#include <QList>

#include <pulse/pulseaudio.h>

#include "audioformat.h"

class PulseDeviceASync : public QIODevice
{
    Q_OBJECT
public:
    explicit PulseDeviceASync(AudioFormat *format,QObject *parent = 0);
    bool open(OpenMode mode);
    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);
    void close();
    QStringList getDevicesNames(QIODevice::OpenMode mode);
    QList<int> getValidSamplesSize();
    QList<pa_source_info> getSourcesDevices();
    QList<pa_sink_info> getSinkDevices();
    static QList<int> getDefaultSamplesRates();
private:
    pa_context *context;
    bool makeContext();
    QString name;
    pa_stream *streamPlayback;
    pa_stream *streamRecord;
    void say(const QString message);
    AudioFormat *format;
    pa_mainloop* mainloop;
    pa_mainloop_api *mainloop_api;
    pa_sample_spec ss;
    bool makeChannelMap(pa_channel_map* map);
    pa_sample_format getSampleFormat();
    pa_channel_map map;
    bool isValidSampleSepcs();
signals:

public slots:

};

#endif // PULSEDEVICEASYNC_H

#endif
