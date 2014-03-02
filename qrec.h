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
    explicit QRec(QObject *parent = 0);
    QStringList getDevicesList(QAudio::Mode type);
    void listDevices();
    void setCodec(const QString codec);
    void setTargetFilePath(const QString filePath);
    void setSourceFilePath(const QString filePath);
    bool isRecording();
    bool setSourceId(const int deviceId);
    bool setSampleRate(const int sampleRate);
    QStringList getSupportedCodec();
    QStringList getSupportedSamplesRates();
    bool startRecAlt();
    QAudioDeviceInfo getAudioDeviceById(const int id, QAudio::Mode type);
    QAudioDeviceInfo getAudioDeviceById(const int id);
    QList<QAudioDeviceInfo> getAllAudioDevice();
    QStringList getAllAudioDevicesNames();
    QAudioDeviceInfo getAudioDeviceByName(const QString name);
    void setAudioOutput(QIODevice *output);
    enum Mode {File = 0,Device = 1,Tcp = 2,Undefined = 3};
    bool setTargetTcp(const QString host,const int port);
    bool setSourceTcp(const QString authorisedHosts,const int port);
    bool setChannelNumber(const int channels);
    int getMaxChannelsCount();
    void setTcpOutputBuffer(const int bufferTimeInMs);

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

signals:
    void stoped();

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
};

#endif // QREC_H
