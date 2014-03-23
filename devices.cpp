#include "devices.h"
#include <QAudioOutput>
#include <QAudioInput>

Devices::Devices(QObject *parent) :
    QObject(parent)
{
    in = 0;
    out = 0;
}
Devices::~Devices() {
    if (in != 0) in->deleteLater();
    if (out != 0) out->deleteLater();
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
void Devices::setFormat(QAudioFormat newFormat) {
    format = newFormat;
}
bool Devices::setInputDevice(const int deviceId) {
    if (deviceId > getDevices(QAudio::AudioInput).count()) return false;
    info = getDevices(QAudio::AudioInput).at(deviceId);
    return true;
}
bool Devices::setOutputDevice(const int deviceId) {
    if (deviceId > getDevices(QAudio::AudioOutput).count()) return false;
    info = getDevices(QAudio::AudioOutput).at(deviceId);
    return true;
}

QIODevice* Devices::getInputDevice() {
    if (in != 0) in->deleteLater();
    in = new QAudioInput(info,info.nearestFormat(format),this);
    return in->start();
}
QIODevice* Devices::getOutputDevice() {
    if (out != 0) out->deleteLater();
    out = new QAudioOutput(info,info.nearestFormat(format),this);
    return out->start();
}
QAudioDeviceInfo Devices::getInputDeviceInfo() {
    return info;
}
