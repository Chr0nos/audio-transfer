#ifndef MANAGER_H
#define MANAGER_H

#include "devices.h"
#include "tcpsink.h"

#ifdef PULSE
    #include "pulse.h"
#endif

#include <QObject>
#include <QIODevice>
#include <QAudioFormat>
#include <QFile>

class Manager : public QObject
{
    Q_OBJECT
public:
    enum Mode {File = 0,Device = 1,Tcp = 2,None = 3,PulseAudio = 4};
    struct tcpConfig {
        QString host;
        int port;
        bool sendConfig;
    };

    struct userConfig {
       Mode modeInput;
       Mode modeOutput;
       QString codec;
       int sampleRate;
       int sampleSize;
       int channels;
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
    QAudioDeviceInfo getInputDeviceInfo();
    static QStringList intListToQStringList(QList<int> source);
    static QStringList getLocalIps(const bool ignoreLocal = true);

private:
    userConfig config;
    bool openInput();
    bool openOutput();
    QIODevice *devIn;
    QIODevice *devOut;
    QAudioFormat format;
    TcpSink *tcpSink;
    Devices in;
    Devices out;
    QFile *fileOut;
    QFile *fileIn;
    QByteArray buffer;
    quint64 bytesCount;
    int deviceIdIn;
    int deviceIdOut;
    bool bisRecording;
    void debugList(const QStringList list);
#ifdef PULSE
    Pulse *pulse;
#endif

signals:
    void tcpTargetConnected();
    void errors(const QString error);
    void stoped();
    void started();
    void debug(const QString message);
public slots:
    void tcpTargetOpened();
    void tcpTargetReady();
    void tcpTargetDisconnected();
    void tcpTargetSockRead(const QString message);
    void transfer();
};

#endif // MANAGER_H
