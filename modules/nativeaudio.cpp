#ifdef MULTIMEDIA

#include "nativeaudio.h"
#include "audioformat.h"

#include <QtMultimedia/QAudioDeviceInfo>
#include <QIODevice>
#include <QDebug>

//Todo: allow output to be selected as input (may not work but in some rares cases it does so...)

/* how to use it
 *  QIODevice *dev = new NativeAudio("test sample",&myFormat,this);
 *  if (dev.open(QIODevice::WriteOnly)) {
 *      //here you can write into dev
 *  }
 * */

NativeAudio::NativeAudio(const QString name,AudioFormat *format, QObject *parent) :
    QIODevice(parent)
{
    say("init");
    //switching from my AudioFormat to QAudioFormat
    this->format.setSampleRate(format->getSampleRate());
    this->format.setSampleSize(format->getSampleSize());
    this->format.setCodec(format->getCodec());
    this->format.setChannelCount(format->getChannelsCount());
    this->name = name;
    in = 0;
    out = 0;
    devIn = NULL;
    devOut = NULL;
    deviceIdIn = -1;
    deviceIdOut = -1;
}
NativeAudio::~NativeAudio() {
    say("deleted");
}

bool NativeAudio::open(OpenMode mode) {
    say("opening");
    bool state = false;
    qDebug() << mode;
    if ((mode == QIODevice::WriteOnly) || (mode == QIODevice::ReadWrite)) state = configureDevice(QAudio::AudioOutput,deviceIdOut);
    if ((mode == QIODevice::ReadOnly) || (mode == QIODevice::ReadWrite)) state = configureDevice(QAudio::AudioInput,deviceIdIn);
    if (state) state = QIODevice::open(mode);
    return state;
}
bool NativeAudio::configureDevice(QAudio::Mode mode, const int deviceId) {
    QAudioDeviceInfo info;
    //if the device is = to -1 (or less) : i will select the default device.
    if (deviceId < 0) {
        if (mode == QAudio::AudioInput) info = QAudioDeviceInfo::defaultInputDevice();
        else if (mode == QAudio::AudioOutput) info = QAudioDeviceInfo::defaultOutputDevice();
    }
    else {
        info = QAudioDeviceInfo::availableDevices(mode).at(deviceId);
    }
    say("requested device: " + QString("[") + QString::number(deviceId) + QString("] ") + info.deviceName());

    if (!info.isFormatSupported(format)) {
        say("unsuported format requested");
        return false;
    }
    if (mode == QAudio::AudioInput) {
        if (in) in->deleteLater();
        in = new QAudioInput(info,format,this);
        in->setObjectName(name);
        devIn = NULL;
        devIn = in->start();
        connect(devIn,SIGNAL(readyRead()),this,SIGNAL(readyRead()));
        connect(devIn,SIGNAL(aboutToClose()),this,SIGNAL(aboutToClose()));
        if (devIn) {
            say("ok for record");
            return true;
        }
    }
    else if (mode == QAudio::AudioOutput) {
        if (out) out->deleteLater();
        out = new QAudioOutput(info,format,this);
        if (!out) {
            say("cannot alocate memory for output");
            return false;
        }
        out->setObjectName(name);
        say("opening device...");
        qDebug() << out << out->format() << out->error();
        devOut = out->start();
        say("device open");
        connect(devOut,SIGNAL(aboutToClose()),this,SIGNAL(aboutToClose()));
        if (devOut) {
            say("ok for playback");
            return true;
        }
    }
    return false;
}

void NativeAudio::close() {
    QIODevice::close();
    if (devOut) devOut->close();
    if (devIn) devIn->close();
    devOut = NULL;
    devIn = NULL;
    say("closed");
}

qint64 NativeAudio::writeData(const char *data, qint64 len) {
    return devOut->write(data,len);
}
qint64 NativeAudio::readData(char *data, qint64 maxlen) {
   return devIn->read(data,maxlen);
}
void NativeAudio::say(const QString message) {
#ifdef DEBUG
    qDebug() << "Native: " + message;
#endif
    emit(debug("Native: " + message));
}
bool NativeAudio::setDeviceId(QAudio::Mode mode, const int id) {
    if (id > QAudioDeviceInfo::availableDevices(mode).count()) return false;
    if (mode == QAudio::AudioInput) this->deviceIdIn = id;
    else if (mode == QAudio::AudioOutput) this->deviceIdOut = id;
    say("changing device to id: " + QString::number(id));
    return true;
}

QStringList NativeAudio::getDevicesNames(QAudio::Mode mode) {
    QStringList devicesNames;
    QList<QAudioDeviceInfo> devices = QAudioDeviceInfo::availableDevices(mode);
    QList<QAudioDeviceInfo>::iterator i;
    for (i = devices.begin();i != devices.end();i++) {
        QAudioDeviceInfo device = *i;
        devicesNames << device.deviceName();
    }
    return devicesNames;
}

#endif
