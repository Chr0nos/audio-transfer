#ifdef MULTIMEDIA

#ifndef NATIVEAUDIO_H
#define NATIVEAUDIO_H

#include <QIODevice>
#include <QString>
#include <QStringList>
#include <QtMultimedia/QAudioInput>
#include <QtMultimedia/QAudioOutput>
#include <QtMultimedia/QAudioFormat>
#include <QtMultimedia/QAudioDeviceInfo>

#include "audioformat.h"
#include "modules/moduledevice.h"

class NativeAudio : public ModuleDevice
{
    Q_OBJECT
public:
    explicit NativeAudio(const QString name, AudioFormat *format, QObject *parent = 0);
    ~NativeAudio();
    bool open(OpenMode mode);
    void close();
    bool setDeviceId(QAudio::Mode mode,const int id);
    static QStringList getDevicesNames(QAudio::Mode mode);
    qint64 bytesAvailable();
    static QAudio::Mode getAudioFlag(const QIODevice::OpenModeFlag mode);
    static ModuleDevice *factory(QString name, AudioFormat *format, void *userData, QObject *parent);
private:
    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);
    QIODevice *devIn;
    QIODevice *devOut;
    QAudioFormat format;
    QString name;
    QAudioOutput *out;
    QAudioInput *in;
    void say(const QString message);
    int deviceIdIn;
    int deviceIdOut;
    bool configureDevice(QAudio::Mode mode, const int deviceId);
    void stateChanged(QAudio::State state);
signals:
    void readyRead();
    void debug(const QString message);
public slots:

};

#endif // NATIVEAUDIO_H

#endif
