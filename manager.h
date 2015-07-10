#ifndef MANAGER_H
#define MANAGER_H

#include "audioformat.h"
#include "circularbuffer.h"

#include "modules/moduledevice.h"
#ifdef MULTIMEDIA
    #include "modules/nativeaudio.h"
#endif
#include "modules/tcpdevice.h"
#include "modules/udpdevice.h"
#include "modules/zerodevice.h"
#include "modules/pipedevice.h"
#include "modules/filedevice.h"

#ifdef PULSE
    #include "modules/pulse.h"
#endif

#ifdef PULSEASYNC
    #include "modules/pulsedeviceasync.h"
#endif

#ifdef PORTAUDIO
    #include "modules/portaudiodevice.h"
#endif

#ifdef ASIO
//    #include "modules/asiodevice.h"
#endif

#include "modules/freqgen.h"

//#include <QtMultimedia/QAudio>
//#include <QtMultimedia/QAudioDeviceInfo>
#include <QObject>
#include <QIODevice>
#include <QMap>

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
        PulseAudioAsync = 8,
        Pipe = 9,
        Raw = 10,
        FreqGen = 11,
        AsIO = 12
    };

    struct userConfig {
       Mode modeInput;
       Mode modeOutput;
       AudioFormat *format;
       struct devicesId {
           int input;
           int output;
       };
       devicesId devices;

       int bufferSize;
       int bufferMaxSize;

       struct Name {
           QString input;
           QString output;
       };
       Name devicesNames;
       struct pipeCfg {
           bool hexMode;
       };
       struct pulseCfg {
           QString target;
       };
       struct rawCfg {
           QIODevice* devIn;
           QIODevice* devOut;
       };
       struct networkCfg {
           QString host;
           qint16 port;
           bool sendConfig;
       };
       struct fileCfg {
           QString output;
           QString input;
           QString *filePath;
       };

       pipeCfg pipe;
       pulseCfg pulse;
       rawCfg raw;
       networkCfg network;
       fileCfg file;
    };

    explicit Manager(QObject *parent = 0);
    ~Manager();
    #ifdef MULTIMEDIA
    static QStringList getDevicesNames(QAudio::Mode mode);
    #endif
    void setUserConfig(userConfig cfg);
    quint64 getTransferedSize();
    bool isRecording();
    static QStringList intListToQStringList(QList<int> source);
    static QStringList getLocalIps(const bool ignoreLocal = true);
    static QMap<Mode,QString> getModesMap();
    static Mode getModeFromString(const QString *name);
    static QString getStringFromMode(const Manager::Mode *mode);

private:
    //pointer of function
    typedef ModuleDevice* (*FactoryFct_t)(QString, AudioFormat*, void*, QObject*);
    struct prepair_cfg
    {
        QString         name;
        Manager::Mode   target;
        QIODevice       *rawDev;
        int             deviceId;
    };
    userConfig config;
    bool openInput();
    bool openOutput();
    QIODevice *devIn;
    QIODevice *devOut;
    AudioFormat *format;
    CircularBuffer *buffer;
    quint64 bytesCount;
    bool bisRecording;
    void debugList(const QStringList list);
    bool prepare(QIODevice::OpenModeFlag mode, QIODevice **device);
    void say(const QString message);
    bool transferChecks();
    void init_cfg(QIODevice::OpenModeFlag mode, prepair_cfg* cfg);
    void init_devptr(QMap<Manager::Mode, FactoryFct_t> *devptr);
    void clean_devices();

signals:
    void errors(const QString error);
    void stoped();
    void started();
    void debug(const QString message);
public slots:
    void transfer();
    void devInClose();
    void devOutClose();
    bool start();
    void stop();
};

#endif // MANAGER_H
