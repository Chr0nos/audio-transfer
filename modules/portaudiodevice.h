#ifdef PORTAUDIO

#ifndef PORTAUDIODEVICE_H
#define PORTAUDIODEVICE_H

#include <QIODevice>
#include <QString>
#include <QTimer>
#include <QList>
#include <portaudio.h>

#include "audioformat.h"
#include "circularbuffer.h"

class PortAudioDevice : public QIODevice
{
    Q_OBJECT
public:
    explicit PortAudioDevice(AudioFormat *format,QObject *parent = 0);
    ~PortAudioDevice();
    bool open(OpenMode mode);
    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);
    static int PaStreamCallback(const void *input, void *output, unsigned long frameCount, const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *userData);
    void close();
    int getDevicesCount();
    QList<PaDeviceInfo*> getDevicesInfo();
    const PaDeviceInfo* getDeviceInfo(const int deviceId);
    QStringList getDevicesNames();
    bool initPa();
    bool setDeviceId(const int deviceId, OpenModeFlag mode);
    static PaSampleFormat getSampleFormat(const int sampleSize);
    int getDeviceIdByName(const QString name);
    bool setDeviceByName(const QString name,QIODevice::OpenModeFlag mode);
    qint64 bytesAvailable();
private:
    void say(const QString message);
    void say(QStringList* list);
    PaStream *stream;
    AudioFormat *format;
    QTimer* timer;
    bool binit;
    int currentDeviceIdInput;
    int currentDeviceIdOutput;
    CircularBuffer *readBuffer;
    void sendRdyRead();
    bool modeAsync;
    int framesPerBuffer;

signals:
    void debug(const QString message);
public slots:

};

#endif // PORTAUDIODEVICE_H

#endif
