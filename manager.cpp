#include "manager.h"
#include "devices.h"
#include "tcpsink.h"

#include <QString>
#include <QIODevice>
#include <QtNetwork/QNetworkInterface>
#include <QDebug>

//todo: this class will replace the whole Rec class (too bloated and nasty to evolut it)

Manager::Manager(QObject *parent) :
    QObject(parent)
{
    config.modeOutput = None;
    config.modeInput = None;
    devIn = 0;
    devOut = 0;
    tcpSink = 0;
    fileOut = 0;
    fileIn = 0;
    deviceIdIn = 0;
    deviceIdOut = 0;
    bytesCount = 0;
#ifdef PULSE
    pulse = 0;
#endif
    bisRecording = false;
    qDebug() << "manager ready";
}
Manager::~Manager() {
    if (devIn) devIn->deleteLater();
    if (fileOut) fileOut->deleteLater();
    if (tcpSink) tcpSink->deleteLater();
    if (fileIn) fileIn->deleteLater();
    if (devOut) devOut->deleteLater();
}

bool Manager::start() {
    debug("Starting manager...");

    //inputs
    if (config.modeInput == None) emit(errors("no input method specifed: abording"));
    else if (config.modeInput == Device) {
        in.setFormat(format);
        in.setInputDevice(deviceIdIn);
        devIn = in.getInputDevice();
        if (!devIn) return false;
        connect(devIn,SIGNAL(readyRead()),this,SLOT(transfer()));
    }
    else if (config.modeInput == File) {
        fileIn = new QFile(config.filePathInput);
        if (!fileIn->exists()) {
            emit(errors("can't open the input file: no such file or directory."));
            return false;
        }
        else if (!fileIn->open(QIODevice::ReadOnly)) {
            emit(errors("Can't open the input file, check the permissions."));
            return false;
        }
        devIn = fileIn;
        connect(fileIn,SIGNAL(readyRead()),this,SLOT(transfer()));
    }
    if (!devIn) return false;
    debug("selected input mode: " + QString::number(config.modeInput));

    //outputs
    qDebug() << "using: " << getAudioConfig();

    if (config.modeOutput == None) emit(errors("no output method specified: abording."));
    else if (config.modeOutput == Tcp) {
        QStringList ips = getLocalIps();
        if (ips.isEmpty()) {
            emit(errors("No local ip:  wait for DHCP or set local ip manualy."));
            return false;
        }
        debug("local ips:");
        debugList(ips);
        tcpSink = new TcpSink(this);
        connect(tcpSink,SIGNAL(connected()),this,SLOT(tcpTargetOpened()));
        connect(tcpSink,SIGNAL(reply(QString)),this,SLOT(tcpTargetSockRead(QString)));
        connect(tcpSink,SIGNAL(disconnected()),this,SLOT(tcpTargetDisconnected()));
        tcpSink->connectToHost(config.tcpTarget.host,config.tcpTarget.port);
    }
    else if (config.modeOutput == Device) {
        out.setFormat(format);
        out.setOutputDevice(deviceIdOut);
        devOut = in.getOutputDevice();
        if (!devOut) emit(errors("can't open output device: " + QString::number(config.devices.output)));
    }
    else if ((config.modeOutput == File) && (!config.filePathOutput.isEmpty())) {
        fileOut = new QFile(config.filePathOutput);
        if (!fileOut->open(QIODevice::WriteOnly)) return false;
        devOut = fileOut;
    }
#ifdef PULSE
    else if (config.modeOutput == PulseAudio) {
        debug("using module: PULSE");
        this->pulse = new Pulse(config.pulseTarget,format,this);
        connect(this->pulse,SIGNAL(say(QString)),this,SIGNAL(debug(QString)));
        devOut = this->pulse->getDevice();
    }
#endif
    else {
        errors("unsuported output mode: " + QString::number(config.modeOutput));
        return false;
}
    if (!devOut) return false;
    debug("selected output mode: " + QString::number(config.modeOutput));

    qDebug() << "started";
    bisRecording = true;
    bytesCount = 0;
    emit(started());
    return true;
}
void Manager::stop() {
    bisRecording = false;
    if (devIn != 0) {
        devIn->close();
        disconnect(devIn,SIGNAL(readyRead()),this,SLOT(transfer()));
    }
    if (config.modeOutput != Tcp) {
        if (devOut != 0) devOut->close();
#ifdef PULSE
        if (config.modeOutput == PulseAudio) {
            pulse->deleteLater();
        }
#endif
    }
    else {
        tcpSink->disconnectFromHost();
        disconnect(tcpSink,SIGNAL(connected()),this,SLOT(tcpTargetOpened()));
        disconnect(tcpSink,SIGNAL(disconnected()),this,SLOT(tcpTargetDisconnected()));
    }
    if ((fileOut != 0) && (fileOut->isOpen())) fileOut->close();
    emit(stoped());
}

QStringList Manager::getDevicesNames(QAudio::Mode mode) {
    return Devices::getDevicesNames(mode);
}
void Manager::setUserConfig(userConfig cfg) {
    if (isRecording()) {
        emit(errors("can't change user configuration while transfering."));
        return;
    }
    config = cfg;
    format.setCodec(config.codec);
    format.setChannelCount(config.channels);
    format.setSampleRate(config.sampleRate);
    format.setSampleSize(config.sampleSize);
}
QString Manager::getAudioConfig() {
    return "samplerate:" + QString::number(config.sampleRate) + " samplesize:" + QString::number(config.sampleSize) + " channels:" + QString::number(config.channels);
}
void Manager::tcpTargetOpened() {
    debug("Tcp connected to remote server");
    qDebug() << "sending configuration to server";
    devOut = tcpSink->getDevice();

    if (config.tcpTarget.sendConfig) {
        //send the client config
        QString srvCfg = getAudioConfig();
        tcpSink->send(&srvCfg);
    }
    emit(tcpTargetConnected());
}
void Manager::tcpTargetReady() {
    transfer();
}
void Manager::transfer() {
    if ((!devOut) || (!devIn) || (!devIn->isOpen()) || (!devOut->isOpen())) {
        debug("manager: stoping record");
        stop();
        return;
    }
    QByteArray data = devIn->readAll();
    bytesCount += data.size();
    const int bsize = buffer.size();
    //if the buffer size is too big: we just drop the datas to prevent memory overflow by this buffer
    if (bsize > config.bufferMaxSize) return;
    buffer.append(data);
    if (bsize >= config.bufferSize) {
        devOut->write(buffer);
        buffer.clear();
        //buffer.remove(0,config.bufferSize);
    }
}
quint64 Manager::getTransferedSize() {
    return bytesCount;
}
void Manager::tcpTargetDisconnected() {
    debug("Tcp disconnected");
    this->stop();
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
void Manager::tcpTargetSockRead(const QString message) {
    debug("server reply: " + message);
}
