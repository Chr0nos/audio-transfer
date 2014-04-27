#include "manager.h"
#include "devices.h"
#include "tcpsink.h"
#include "zerodevice.h"

#include <QString>
#include <QIODevice>
#include <QtNetwork/QNetworkInterface>
#include <QDebug>

//todo: make evry output a child of QIODevice to prevent special case each time

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
    qDebug() << "deleting manager";
    if (devIn) devIn->deleteLater();
    if (fileOut) fileOut->deleteLater();
    if (tcpSink) tcpSink->deleteLater();
    if (fileIn) fileIn->deleteLater();
    if (devOut) devOut->deleteLater();
}

bool Manager::prepareSource() {
    //input
    devIn = 0;
    if (config.modeInput == None) emit(errors("no input method specifed: abording"));
    else if (config.modeInput == Device) {
        in.setFormat(format);
        in.setInputDevice(deviceIdIn);
        devIn = in.getInputDevice();
        if (!devIn) return false;
    }
    else if (config.modeInput == File) {
        fileIn = new QFile(config.filePathInput);
        if (!fileIn->exists()) {
            emit(errors("can't open the input file: no such file or directory."));
            return false;
        }
        devIn = fileIn;
    }
    else if (config.modeInput == Zero) {
        devIn = new ZeroDevice(&format,this);
    }
#ifdef PULSE
    else if (config.modeInput == PulseAudio) {
        devIn = new PulseDevice("Audio-Transfer-Client",NULL,format,this);
    }
#endif
    if (!devIn) return false;

    //connecting the source to transfer function (main function)
    connect(devIn,SIGNAL(readyRead()),this,SLOT(transfer()));
    debug("source connected");

    //opening source
    if (!devIn->open(QIODevice::ReadOnly)) {
        emit(errors("can't open input device: abording"));
        return false;
    }
    else debug("source device opened");


    debug("selected input mode: " + QString::number(config.modeInput));
    return true;
}
bool Manager::prepareOutput() {
    //output
    devOut = 0;
    qDebug() << "using: " << getAudioConfig();

    if (config.modeOutput == None) {
        emit(errors("no output method specified: abording."));
        return false;
    }
    else if (config.modeOutput == Tcp) {
        QStringList ips = getLocalIps();
        if (ips.isEmpty()) {
            emit(errors("No local ip:  wait for DHCP or set local ip manualy."));
            return false;
        }
        debug("local ips:");
        debugList(ips);
        tcpSink = new TcpSink(this);
        debug("tcp sink created");
        connect(tcpSink,SIGNAL(connected()),this,SLOT(tcpTargetOpened()));
        connect(tcpSink,SIGNAL(reply(QString)),this,SLOT(tcpTargetSockRead(QString)));
        connect(tcpSink,SIGNAL(disconnected()),this,SLOT(tcpTargetDisconnected()));
        debug("tcp sink connected to manager, connecting to host...");
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
        devOut = new PulseDevice("Audio-Transfer-Client",config.pulseTarget,format,this);
    }
#endif
    else if (config.modeOutput == Zero) {
        devOut = new ZeroDevice(&format,this);
    }
    else {
        errors("unsuported output mode: " + QString::number(config.modeOutput));
        return false;
    }
    if (!devOut) return false;
    if (!devOut->isOpen()) {
        if (!devOut->open(QIODevice::WriteOnly)) {
            emit(errors("cannot open output device in writeOnly"));
            return false;
        }
    }
    debug("selected output mode: " + QString::number(config.modeOutput));
    return true;
}

bool Manager::start() {
    debug("Starting manager...");
    if (!prepareOutput()) {
        errors("failed to start output");
        return false;
    }

    debug("output: ready");
    if (!prepareSource()) {
        errors("failed to start input");
        return false;
    }
    debug("input ready");

    qDebug() << "started";
    bisRecording = true;
    bytesCount = 0;
    emit(started());
    //transfer();
    return true;
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
    else if (tcpSink) {
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
    format = cfg.format;
}
QString Manager::getAudioConfig() {
    return "samplerate:" + QString::number(config.format.sampleRate()) + " samplesize:" + QString::number(config.format.sampleSize()) + " channels:" + QString::number(config.format.channelCount());
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
    const int bsize = buffer.size();
    //if the buffer size is too big: we just drop the datas to prevent memory overflow by this buffer
    if (bsize > config.bufferMaxSize) return;
    buffer.append(data);
    if (bsize >= config.bufferSize) {
        devOut->write(buffer);
        buffer.clear();
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
void Manager::devOutClose() {
    debug("output closed");
}
void Manager::devInClose() {
    debug("input closed");
}
