#ifdef PULSEASYNC

#ifndef PULSEDEVICEASYNC_H
#define PULSEDEVICEASYNC_H

#include <QIODevice>
#include <QStringList>
#include <QList>

#include <pulse/pulseaudio.h>

#include "circularbuffer.h"
#include "audioformat.h"

class PulseDeviceASync : public QIODevice
{
    Q_OBJECT
public:
    explicit PulseDeviceASync(AudioFormat *format, const QString serverHost, QObject *parent = 0);
    ~PulseDeviceASync();
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
    static void stream_state_callback(pa_stream* stream,void* userdata);
    static void stream_read_callback(pa_stream* stream,size_t len,void* userdata);
    qint64 bytesAvailable();
    void setObjectName(const QString &name);
private:
    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);
    pa_context *context;
    bool makeContext();
    bool makeChannelMap(pa_channel_map* map);
    bool makeStream();
    pa_stream *stream;
    AudioFormat *format;
    pa_threaded_mainloop* mainloop;
    pa_mainloop_api *mainloop_api;
    pa_sample_spec ss;
    pa_sample_format getSampleFormat();
    pa_channel_map map;
    bool isValidSampleSepcs();
    QString serverHost;
    static QString stateToString(pa_context_state_t state);
    CircularBuffer* readBuffer;

signals:
    void debug(const QString message);
    void readyWrite();
public slots:
private slots:
    void say(const QString message);
};

#endif // PULSEDEVICEASYNC_H

#endif
