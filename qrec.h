#ifndef QREC_H
#define QREC_H

#include <QObject>
#include <QStringList>
#include <QtMultimedia/QAudioBuffer>
#include <QtMultimedia/QAudioProbe>
#include <QtMultimedia/QAudioEncoderSettings>
#include <QFile>
#include <QtMultimedia/QAudioInput>
#include <QTcpSocket>
#include <QAudioDecoder>
#include <QTcpServer>

//todo: private: QByteArray tcpOutputBuffer;

class QRec : public QObject
{
    Q_OBJECT
public:
    enum Mode {File = 0,Device = 1,Tcp = 2,Undefined = 3};
    struct userConfig {
        int channels;
        int sampleRate;
        int sampleSize;
        QString codec;
    };

    explicit QRec(QObject *parent = 0);
    QStringList getDevicesList(QAudio::Mode type);
    QStringList getDevicesList();
    void listDevices();
    void setTargetFilePath(const QString filePath);
    void setSourceFilePath(const QString filePath);
    bool isRecording();
    bool setSourceId(const int deviceId);
    QStringList getSupportedCodec();
    QStringList getSupportedSamplesRates();
    bool startRecAlt();
    QAudioDeviceInfo getAudioDeviceById(const int id, QAudio::Mode type);
    QAudioDeviceInfo getAudioDeviceById(const int id);
    QList<QAudioDeviceInfo> getAllAudioDevice();
    QStringList getAllAudioDevicesNames();
    QAudioDeviceInfo getAudioDeviceByName(const QString name);
    void setAudioOutput(QIODevice *output);
    bool setTargetTcp(const QString host,const int port);
    bool setSourceTcp(const QString authorisedHosts,const int port);
    int getMaxChannelsCount();
    void setTcpOutputBuffer(const int bufferTimeInMs);
    void setUserConfig(userConfig config);
    quint64 getReadedData();

private:
    QString codec;
    QFile targetFile;
    QFile *sourceFile;
    QString source;
    int sampleRate;
    int channels;
    QAudioProbe *probe;
    QAudioEncoderSettings *audioSettings;
    QIODevice *devIn;
    QIODevice *devOut;
    quint64 readedData;
    bool bIsRecording;
    QTcpSocket *tcp;
    QAudioInput *audioInput;
    void decodeSourceFile (QFile *filePath);
    QAudioDecoder *decoder;
    QRec::Mode currentSourceMode;
    QRec::Mode currentTargetMode;
    QTcpServer *tcpSourceServ;
    QTcpSocket *tcpSource;
    QAudioDeviceInfo currentSourceInfo;
    int tcpOutputBufferSize;
    QByteArray tcpOutputBuffer;
    userConfig config;

signals:
    void stoped();
    void targetConnected();

public slots:
    void redirectBuffer(QAudioBuffer buffer);
    void redirectFlushed();
    void volumeChanged(qreal volume);
    void deviceChanged(QString newDevice);
    void flusher();
    void stopRec();
    void deviceStateChanged(QAudio::State state);
    void readyRead();
    void capture();
    void decodeReadReady();
    void tcpDisconnect();
    void tcpNewConnection();
    void tcpSourceDisconnected();
    void tcpTargetConnected();
};

#endif // QREC_H
