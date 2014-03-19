#include "devices.h"

Devices::Devices(QObject *parent) :
    QObject(parent)
{
}
QList<QAudioDeviceInfo> Devices::getDevices(QAudio::Mode mode) {
    return QAudioDeviceInfo::availableDevices(mode);
}
QStringList Devices::getDevicesNames(QAudio::Mode mode) {
    QStringList devicesNames;
    QList<QAudioDeviceInfo> devices = QAudioDeviceInfo::availableDevices(mode);
    QList<QAudioDeviceInfo>::iterator i;
    for (i = devices.begin();i != devices.end();i++) {
        QAudioDeviceInfo device = *i;
        devicesNames << device.deviceName();
    }
    return devicesNames;
}
