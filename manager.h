#ifndef MANAGER_H
#define MANAGER_H

#include "audioformat.h"


#include "modules/nativeaudio.h"
#include "modules/tcpdevice.h"
#include "modules/udpdevice.h"
#include "modules/zerodevice.h"

#ifdef PULSE
    #include "modules/pulse.h"
    #include "modules/pulsedeviceasync.h"
#endif

#ifdef PORTAUDIO
    #include "modules/portaudiodevice.h"
#endif

#include <QtMultimedia/QAudio>
#include <QtMultimedia/QAudioDeviceInfo>
#include <QObject>
#include <QIODevice>

class Manager : public QObject
{
    Q_OBJECT
public:
    enum Mode {
        File = 0,
        Device = 1,
        Tcp = 2,
        None = 3,
        PulseAudio = 4,
        Zero = 5,
        Udp = 6,
        PortAudio = 7,
        PulseAudioAsync = 8
    };
    struct tcpConfig {
        QString host;
        int port;
        bool sendConfig;
    };
    struct portAudioSpecs {
        int deviceIdInput;
        int deviceIdOutput;
    };

    struct userConfig {
       Mode modeInput;
       Mode modeOutput;
       AudioFormat *format;
       tcpConfig tcpTarget;
       struct devicesId {
           int input;
           int output;
       };
       devicesId devices;
       QString filePathOutput;
       QString filePathInput;
       int bufferSize;
       int bufferMaxSize;
       QString pulseTarget;
       portAudioSpecs portAudio;
    };

    explicit Manager(QObject *parent = 0);
    ~Manager();
    bool start();
    void stop();
    static QStringList getDevicesNames(QAudio::Mode mode);
    void setUserConfig(userConfig cfg);
    QString getAudioConfig();
    quint64 getTransferedSize();
    bool isRecording();
    static QStringList intListToQStringList(QList<int> source);
    static QStringList getLocalIps(const bool ignoreLocal = true);

private:
    userConfig config;
    bool openInput();
    bool openOutput();
    QIODevice *devIn;
    QIODevice *devOut;
    AudioFormat *format;
    QByteArray buffer;
    quint64 bytesCount;
    bool bisRecording;
    void debugList(const QStringList list);
    bool prepare(QAudio::Mode mode, QIODevice **device);

signals:
    void errors(const QString error);
    void stoped();
    void started();
    void debug(const QString message);
public slots:
    void transfer();
    void devInClose();
    void devOutClose();
};

#endif // MANAGER_H
