#include "manager.h"

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
    say("manager ready");
}
Manager::~Manager() {
    say("deleting manager");
    if (devIn) devIn->deleteLater();
    if (devOut) devOut->deleteLater();
    delete(format);
}
bool Manager::prepare(QAudio::Mode mode, QIODevice **device) {
    if ((device) && ((*device))) (**device).deleteLater();
    *device = NULL;
    QString name;
    QString* filePath;
    Manager::Mode target = Manager::None;
    QIODevice::OpenModeFlag flag;
    QIODevice *rawDev;
    int deviceId;
    if (mode == QAudio::AudioInput) {
        target = config.modeInput;
        flag = QIODevice::ReadOnly;
        deviceId = config.devices.input;
        filePath = &config.filePathInput;
        rawDev = config.devIn;
        if (!config.devicesNames.input.isEmpty()) name = config.devicesNames.input;
    }
    else if (mode == QAudio::AudioOutput) {
        target = config.modeOutput;
        flag = QIODevice::WriteOnly;
        deviceId = config.devices.output;
        filePath = &config.filePathOutput;
        rawDev = config.devOut;
        if (!config.devicesNames.output.isEmpty()) name = config.devicesNames.output;
    }

    switch (target) {
        case Manager::Device: {
            NativeAudio *native = new NativeAudio(name,format,this);
            connect(native,SIGNAL(debug(QString)),this,SIGNAL(debug(QString)));
            if (!native->setDeviceId(mode,deviceId)) {
                delete(native);
                return false;
            }
            *device = native;
            break;
        }
        case Manager::Tcp: {
            TcpDevice* tcpDevice = new TcpDevice(config.tcpTarget.host,config.tcpTarget.port,format,config.tcpTarget.sendConfig,this);
            connect(tcpDevice,SIGNAL(debug(QString)),this,SIGNAL(debug(QString)));
            *device = tcpDevice;
            break;
        }
        case Manager::Udp: {
            UdpDevice *udpDevice = new UdpDevice(config.tcpTarget.host,config.tcpTarget.port,format,config.tcpTarget.sendConfig,this);
            connect(udpDevice,SIGNAL(debug(QString)),this,SIGNAL(debug(QString)));
            *device = udpDevice;
            break;
        }
#ifdef PULSE
        case Manager::PulseAudio: {
            PulseDevice *pulseDevice = new PulseDevice(name,config.pulseTarget,format,this);
            connect(pulseDevice,SIGNAL(debug(QString)),this,SIGNAL(debug(QString)));
            *device = pulseDevice;
            break;
        }
#ifdef PULSEASYNC
        case Manager::PulseAudioAsync: {
            *device = new PulseDeviceASync(format,config.pulseTarget,this);
            break;
        }
#else
        case Manager::PulseAudioAsync:
            return false;
            break;
#endif
#else
    case Manager::PulseAudioAsync:
            return false;
            break;
    case Manager::PulseAudio:
            //in normal case: this condition will NEVER appens: the ui will not send PulseAudio is the module is not built in: but: security before evrythink.
            emit(errors("pulse audio output requested but ATC was not compiled with pa module"));
            return false;
            break;
#endif
        case Manager::Zero:
            *device = new ZeroDevice(format,this);
            break;
        case Manager::None:
            *device = NULL;
            break;
         case Manager::File:
            *device = new QFile(*filePath);
            break;
         case Manager::Pipe: {
            PipeDevice *pipeDevice = new PipeDevice(name + "PipeDevice socket",this);
            connect(pipeDevice,SIGNAL(debug(QString)),this,SIGNAL(debug(QString)));
            *device = pipeDevice;
            break;
         }
#ifdef PORTAUDIO
         case Manager::PortAudio: {
            PortAudioDevice* api = new PortAudioDevice(format,this);
            if (mode == QAudio::AudioInput) api->setDeviceId(config.portAudio.deviceIdInput,QIODevice::ReadOnly);
            else if (mode == QAudio::AudioOutput) api->setDeviceId(config.portAudio.deviceIdOutput,QIODevice::WriteOnly);
            connect(api,SIGNAL(debug(QString)),this,SIGNAL(debug(QString)));

            *device = api;
            break;
        }
#else
         case Manager::PortAudio:
            return false;
#endif
         case Manager::Raw:
            *device = rawDev;
            break;
    }
    if (!*device) return false;

    if (mode == QAudio::AudioInput) connect(*device,SIGNAL(readyRead()),this,SLOT(transfer()));

    //if a name is available lets assign it to the new device
    if (!name.isEmpty()) (**device).setObjectName(name);

    //in raw mode: we just dont open the device
    if (target == Manager::Raw) return true;
    return (**device).open(flag);
}

bool Manager::start() {
    say("Starting manager...");

    if (!prepare(QAudio::AudioOutput,&devOut)) {
        emit(errors("failed to  start output"));
        emit(stoped());
    }
    else if (!prepare(QAudio::AudioInput,&devIn)) {
        emit(errors("failed to start input"));
        emit(stoped());
    }
    else {
        emit(say("devices ok"));
        say("started");
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
        delete(devIn);
        devIn = NULL;
    }
    if (devOut) {
        devOut->close();
        delete(devOut);
        devOut = NULL;
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

void Manager::transfer() {
    //qDebug() << "transfer!" << devIn << devIn->isOpen();
    if ((!devOut) || (!devIn) || (!devIn->isOpen()) || (!devOut->isOpen())) {
        say("manager: transfer: stoping");
        qDebug() << "devOut: " << devOut;
        if (!devOut) say("manager: transfer: devOut is null");
        else if (!devOut->isOpen()) say("manager: transfer: devOut is closed");

        qDebug() << "devIn: " << devIn;
        if (!devIn) say("manager: transfer: devIn is null");
        else if (!devIn->isOpen()) say("manager: transfer: devIn is closed");

        say("manager: stoping record");
        stop();
        return;
    }
    QByteArray data = devIn->readAll();
    //QByteArray data = devIn->read(devIn->bytesAvailable());

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
QStringList Manager::intListToQStringList(QList<int> source) {
    QStringList result;
    QList<int>::iterator i;
    for (i = source.begin();i != source.end();i++) {
        result << QString::number(*i);
    }
    return result;
}
/*
 * Comment for removal in the future... (or move in the TcpDevice class)
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
*/
void Manager::debugList(const QStringList list) {
    int count = 0;
    foreach (QString x,list) {
        debug("[" + QString::number(count++) + "] " + x);
    }
}
void Manager::devOutClose() {
    say("output closed");
    stop();
}
void Manager::devInClose() {
    say("input closed");
    stop();
}
void Manager::say(const QString message) {
    emit(debug("Manager: " + message));
}
QMap<Manager::Mode,QString> Manager::getModesMap() {
    QMap<Mode,QString> map;
    map[Manager::File] = "file";
#ifdef MULTIMEDIA
    map[Manager::Device] = "native";
#endif
    map[Manager::None] = "none";
    map[Manager::Zero] = "zero";
#ifdef PORTAUDIO
    map[Manager::PortAudio] = "portaudio";
#endif
#ifdef PULSE
    map[Manager::PulseAudio] = "pulseaudio";
#endif
#ifdef PULSEASYNC
    map[Manager::PulseAudioAsync] = "pulseasync";
#endif
    map[Manager::Tcp] = "tcp";
    map[Manager::Udp] = "udp";
    map[Manager::Raw] = "raw";
    map[Manager::Pipe] = "pipe";
    return map;
}
Manager::Mode Manager::getModeFromString(const QString *name) {
    QMap<Manager::Mode,QString>::iterator i;
    QMap<Manager::Mode,QString> map = Manager::getModesMap();
    for (i = map.begin() ; i != map.end() ; i++) {
        if (*name == i.value()) return i.key();
    }
    return Manager::None;
}
QString Manager::getStringFromMode(const Manager::Mode *mode) {
    QMap<Manager::Mode,QString> map = getModesMap();
    QMap<Manager::Mode,QString>::iterator i;
    for (i = map.begin() ; i != map.end() ; i++) {
        if (i.key() == *mode) return i.value();
    }
    return QString("none");
}
