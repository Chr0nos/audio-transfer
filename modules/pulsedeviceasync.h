#ifdef PULSEASYNC

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
    explicit PulseDeviceASync(AudioFormat *format, const QString serverHost, QObject *parent = 0);
    bool open(OpenMode mode);
    void close();
    QStringList getDevicesNames(QIODevice::OpenMode mode);
    QList<int> getValidSamplesSize();
    QList<pa_source_info> getSourcesDevices();
    QList<pa_sink_info> getSinkDevices();
    static QList<int> getDefaultSamplesRates();
    static void getSourcesDevices_cb(pa_context *c,const pa_source_info *i,int eol,void *userdata);
    static void getSinkDevices_cb(pa_context *c,const pa_sink_info* i,int eol,void *userdata);
    static void context_state_callback(pa_context *c, void *userdata);
private:
    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);
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
    QString serverHost;
signals:
    void debug(const QString message);
public slots:

};

#endif // PULSEDEVICEASYNC_H

#endif
