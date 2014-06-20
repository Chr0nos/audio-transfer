#include "manager.h"
#include "devices.h"

#include "modules/nativeaudio.h"
#include "modules/tcpdevice.h"
#include "modules/udpdevice.h"
#include "modules/zerodevice.h"
#include "tcpsink.h"

#include <QString>
#include <QIODevice>
#include <QtNetwork/QNetworkInterface>
#include <QFile>
#include <QDebug>

Manager::Manager(QObject *parent) :
    QObject(parent)
{
    config.modeOutput = None;
    config.modeInput = None;
    devIn = NULL;
    devOut = NULL;
    bytesCount = 0;
    format = new AudioFormat();
    bisRecording = false;
    qDebug() << "manager ready";
}
Manager::~Manager() {
    qDebug() << "deleting manager";
    if (devIn) devIn->deleteLater();
    if (devOut) devOut->deleteLater();
    delete(format);
}
bool Manager::prepare(QAudio::Mode mode, QIODevice **device) {
    if ((*device)) (**device).deleteLater();
    *device = NULL;
    QString name = "Audio-Transfer-Client";
    QString filePath;
    Manager::Mode target = Manager::None;
    QIODevice::OpenModeFlag flag;
    int deviceId;
    if (mode == QAudio::AudioInput) { target = config.modeInput; flag = QIODevice::ReadOnly; deviceId = config.devices.input; name.append(" capture"); }
    else if (mode == QAudio::AudioOutput) { target = config.modeOutput; flag = QIODevice::WriteOnly; deviceId = config.devices.output; name.append(" playback"); }

    switch (target) {
        case Manager::Device:
            NativeAudio *native;
            native = new NativeAudio(name,format,this);
            if (!native->setDeviceId(mode,deviceId)) return false;
            *device = native;
            break;
        case Manager::Tcp:
            *device = new TcpDevice(config.tcpTarget.host,config.tcpTarget.port,format,this);
            break;
        case Manager::Udp:
            *device = new UdpDevice(config.tcpTarget.host,config.tcpTarget.port,format,this);
            break;
#ifdef PULSE
        case Manager::PulseAudio:
            *device = new PulseDevice(name,config.pulseTarget,format,this);
            break;
#endif
        case Manager::Zero:
            *device = new ZeroDevice(format,this);
            break;
        case Manager::None:
            *device = NULL;
            break;
         case Manager::File:
            *device = new QFile(filePath);
            break;
    }
    if (!*device) return false;

    if (mode == QAudio::AudioInput) connect(*device,SIGNAL(readyRead()),this,SLOT(transfer()));
    return (**device).open(flag);
}

bool Manager::start() {
    debug("Starting manager...");

    if (!prepare(QAudio::AudioOutput,&devOut)) emit(errors("failed to  start output"));
    else if (!prepare(QAudio::AudioInput,&devIn)) emit(errors("failed to start input"));
    else {
        emit(debug("devices ok"));
        qDebug() << "started";
        bisRecording = true;
        bytesCount = 0;
        emit(started());
        //transfer();
        return true;
    }
    return false;
}
void Manager::stop() {
    bisRecording = false;
    if (devIn) {
        devIn->close();
        disconnect(devIn,SIGNAL(readyRead()),this,SLOT(transfer()));
    }
    if (config.modeOutput != Tcp) {
        if (devOut) devOut->close();
    }
    emit(stoped());
}

QStringList Manager::getDevicesNames(QAudio::Mode mode) {
    return NativeAudio::getDevicesNames(mode);
}
void Manager::setUserConfig(userConfig cfg) {
    if (isRecording()) {
        emit(errors("can't change user configuration while transfering."));
        return;
    }
    config = cfg;
    format = cfg.format;
}
QString Manager::getAudioConfig() {
    return "samplerate:" + QString::number(config.format->getSampleRate()) + " samplesize:" + QString::number(config.format->getSampleSize()) + " channels:" + QString::number(config.format->getChannelsCount());
}

void Manager::transfer() {
    //qDebug() << "transfer!" << devIn << devIn->isOpen();
    if ((!devOut) || (!devIn) || (!devIn->isOpen()) || (!devOut->isOpen())) {
        qDebug() << "manager: transfer: stoping";
        qDebug() << "devOut: " << devOut;
        if (!devOut) qDebug() << "manager: transfer: devOut is null";
        else if (!devOut->isOpen()) qDebug() << "manager: transfer: devOut is closed";

        qDebug() << "devIn: " << devIn;
        if (!devIn) qDebug() << "manager: transfer: devIn is null";
        else if (!devIn->isOpen()) qDebug() << "manager: transfer: devIn is closed";

        debug("manager: stoping record");
        stop();
        return;
    }
    QByteArray data = devIn->readAll();
    bytesCount += data.size();
    //in case of no buffer usage we dont copy data to buffer (better performance)
    if (!config.bufferSize) {
        devOut->write(data);
    }
    else {
        //if the buffer size is too big: we just drop the datas to prevent memory overflow by this buffer
        const int bsize = buffer.size();
        if (bsize > config.bufferMaxSize) return;
        buffer.append(data);
        if (bsize >= config.bufferSize) {
            devOut->write(buffer);
            buffer.clear();
        }
    }
}
quint64 Manager::getTransferedSize() {
    return bytesCount;
}
bool Manager::isRecording() {
    return bisRecording;
}
QAudioDeviceInfo Manager::getInputDeviceInfo() {
    return in.getInputDeviceInfo();
}
QStringList Manager::intListToQStringList(QList<int> source) {
    QStringList result;
    QList<int>::iterator i;
    for (i = source.begin();i != source.end();i++) {
        result << QString::number(*i);
    }
    return result;
}
QStringList Manager::getLocalIps(const bool ignoreLocal) {
    QStringList ips;
    QStringList localhost;
    localhost << "127.0.0.1" << "::1";
    QList<QHostAddress> addresses = QNetworkInterface::allAddresses();
    QList<QHostAddress>::iterator i;
    for (i = addresses.begin();i != addresses.end();i++) {
        QHostAddress &add = *i;
        QString addr = add.toString();
        if (!ignoreLocal) ips << addr;
        else if (localhost.contains(addr));
        else ips << addr;
    }
    return ips;
}
void Manager::debugList(const QStringList list) {
    int count = 0;
    foreach (QString x,list) {
        debug("[" + QString::number(count++) + "] " + x);
    }
}
void Manager::devOutClose() {
    debug("output closed");
    stop();
}
void Manager::devInClose() {
    debug("input closed");
    stop();
}
