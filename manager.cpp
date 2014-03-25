#include "manager.h"
#include "devices.h"
#include "tcpsink.h"

#include <QString>
#include <QIODevice>
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
    bisRecording = false;

    qDebug() << "manager ready";
}
Manager::~Manager() {
    if (devIn) devIn->deleteLater();
    if (devOut) devOut->deleteLater();
    if (fileOut) fileOut->deleteLater();
    if (tcpSink) tcpSink->deleteLater();
    if (fileIn) fileIn->deleteLater();
}

bool Manager::start() {
    //outputs
    qDebug() << "using: " << getAudioConfig();

    if (config.modeOutput == None) emit(errors("no output method specified: abording."));
    else if (config.modeOutput == Tcp) {
        tcpSink = new TcpSink(this);
        connect(tcpSink,SIGNAL(connected()),this,SLOT(tcpTargetOpened()));
        connect(tcpSink,SIGNAL(disconnected()),this,SLOT(tcpTargetDisconnected()));
        tcpSink->connectToHost(config.tcpTarget.host,config.tcpTarget.port);
        devOut = tcpSink->getDevice();
    }
    else if (config.modeOutput == Device) {
        out.setFormat(format);
        out.setOutputDevice(deviceIdOut);
        devOut = in.getOutputDevice();
        if (!devOut) emit(errors("can't open input device: " + QString::number(config.devices.output)));
    }
    else if ((config.modeOutput == File) && (!config.filePathOutput.isEmpty())) {
        fileOut = new QFile(config.filePathOutput);
        if (!fileOut->open(QIODevice::WriteOnly)) return false;
        devOut = fileOut;
    }
    else return false;
    if (!devOut) return false;

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
    qDebug() << "started";
    bisRecording = true;
    return true;
}
void Manager::stop() {
    bisRecording = false;
    devIn->close();
    if (config.modeOutput != Tcp) devOut->close();
    else tcpSink->disconnectFromHost();
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
    qDebug() << "sending configuration to server";
    devOut = tcpSink->getDevice();
    //send the client config
    QString srvCfg = getAudioConfig();
    tcpSink->send(&srvCfg);
    emit(tcpTargetConnected());
}
void Manager::tcpTargetReady() {
    transfer();
}
void Manager::transfer() {
    if ((!devOut) || (!devIn) || (!devIn->isOpen()) || (!devOut->isOpen())) {
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
        //devOut << buffer;
        buffer.remove(0,config.bufferSize);
    }
}
quint64 Manager::getTransferedSize() {
    return bytesCount;
}
void Manager::tcpTargetDisconnected() {
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
