#ifndef DEVICES_H
#define DEVICES_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QList>
#include <QAudioDeviceInfo>
#include <QtMultimedia/QAudio>

class Devices : public QObject
{
    Q_OBJECT
public:
    explicit Devices(QObject *parent = 0);
    static QList<QAudioDeviceInfo> getDevices(QAudio::Mode mode);
    static QStringList getDevicesNames(QAudio::Mode mode);

signals:

public slots:

};

#endif // DEVICES_H
