#ifndef DEVICES_H
#define DEVICES_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QList>
#include <QAudioDeviceInfo>
#include <QtMultimedia/QAudio>
#include <QtMultimedia/QAudioInput>
#include <QtMultimedia/QAudioOutput>
#include <QIODevice>

class Devices : public QObject
{
    Q_OBJECT
public:
    explicit Devices(QObject *parent = 0);
    ~Devices();
    static QList<QAudioDeviceInfo> getDevices(QAudio::Mode mode);
    static QStringList getDevicesNames(QAudio::Mode mode);
    void setFormat(QAudioFormat newFormat);
    bool setInputDevice(const int deviceId);
    bool setOutputDevice(const int deviceId);
    QIODevice* getInputDevice();
    QIODevice* getOutputDevice();
    QAudioDeviceInfo getInputDeviceInfo();

private:
    QAudioInput *in;
    QAudioOutput *out;
    QAudioDeviceInfo info;
    QAudioFormat format;

signals:

public slots:

};

#endif // DEVICES_H
