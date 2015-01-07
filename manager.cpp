#include "manager.h"
#include "size.h"

#include <QString>
#include <QIODevice>
#include <QtNetwork/QNetworkInterface>
#include <QFile>

Manager::Manager(QObject *parent) :
    QObject(parent)
{
    config.modeOutput = None;
    config.modeInput = None;
    devIn = NULL;
    devOut = NULL;
    buffer = NULL;
    bytesCount = 0;
    format = new AudioFormat();
    bisRecording = false;
    say("manager ready");
}
Manager::~Manager() {
    say("deleting manager");
    if (devIn) delete(devIn);
    if (devOut) delete(devOut);
    if (buffer) delete(buffer);
    delete(format);
}
bool Manager::prepare(QIODevice::OpenModeFlag mode, QIODevice **device) {
    if ((device) && ((*device))) {
        (**device).disconnect();
        delete(*device);
    }
    *device = NULL;
    QString name;
    QString* filePath;
    Manager::Mode target = Manager::None;
    QIODevice *rawDev;
    int deviceId;
    if (mode == QIODevice::ReadOnly) {
        target = config.modeInput;
        deviceId = config.devices.input;
        filePath = &config.file.input;
        rawDev = config.raw.devIn;
        if (!config.devicesNames.input.isEmpty()) name = config.devicesNames.input;
    }
    else if (mode == QIODevice::WriteOnly) {
        target = config.modeOutput;
        deviceId = config.devices.output;
        filePath = &config.file.output;
        rawDev = config.raw.devOut;
        if (!config.devicesNames.output.isEmpty()) name = config.devicesNames.output;
    }

    switch (target) {
#ifdef MULTIMEDIA
        case Manager::Device: {
            NativeAudio *native = new NativeAudio(name,format,this);
            connect(native,SIGNAL(debug(QString)),this,SIGNAL(debug(QString)));
            if (!native->setDeviceId(NativeAudio::getAudioFlag(mode),deviceId)) {
                delete(native);
                return false;
            }
            *device = native;
            break;
        }
#else
        case Manager::Device:
            (void) deviceId;
            say("not compiled with QtMultimedia support");
            return false;
#endif
        case Manager::Tcp: {
            TcpDevice* tcpDevice = new TcpDevice(config.network.host,config.network.port,format,config.network.sendConfig,this);
            connect(tcpDevice,SIGNAL(debug(QString)),this,SIGNAL(debug(QString)));
            *device = tcpDevice;
            break;
        }
        case Manager::Udp: {
            UdpDevice *udpDevice = new UdpDevice(config.network.host,config.network.port,format,config.network.sendConfig,this);
            connect(udpDevice,SIGNAL(debug(QString)),this,SIGNAL(debug(QString)));
            *device = udpDevice;
            break;
        }
#ifdef PULSE
        case Manager::PulseAudio: {
            PulseDevice *pulseDevice = new PulseDevice(name,config.pulse.target,format,this);
            connect(pulseDevice,SIGNAL(debug(QString)),this,SIGNAL(debug(QString)));
            *device = pulseDevice;
            break;
        }
#else
    case Manager::PulseAudio:
            //in normal case: this condition will NEVER appens: the ui will not send PulseAudio is the module is not built in: but: security before evrythink.
            emit(errors("pulse audio output requested but ATC was not compiled with pa module"));
            return false;
            break;
#endif

#ifdef PULSEASYNC
        case Manager::PulseAudioAsync: {
            PulseDeviceASync* pulse = new PulseDeviceASync(format,config.pulse.target,this);
            pulse->setObjectName("Audio-Transfer");
            connect(pulse,SIGNAL(debug(QString)),this,SIGNAL(debug(QString)));
            *device = pulse;
            break;
        }
#else
        case Manager::PulseAudioAsync:
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
            if (config.pipe.hexMode) pipeDevice->setHexOutputEnabled(true);
            *device = pipeDevice;
            break;
         }
#ifdef PORTAUDIO
         case Manager::PortAudio: {
            PortAudioDevice* api = new PortAudioDevice(format,this);
            if (mode == QIODevice::ReadOnly) api->setDeviceId(config.portAudio.deviceIdInput,mode);
            else if (mode == QIODevice::WriteOnly) api->setDeviceId(config.portAudio.deviceIdOutput,mode);
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
         case Manager::FreqGen:
            Freqgen *freqgen = new Freqgen(format,this);
            connect(freqgen,SIGNAL(debug(QString)),this,SIGNAL(debug(QString)));
            *device = freqgen;
            break;
    }
    if (!*device) return false;

    if (mode == QIODevice::ReadOnly) connect(*device,SIGNAL(readyRead()),this,SLOT(transfer()));

    //if a name is available lets assign it to the new device
    if (!name.isEmpty()) (**device).setObjectName(name);

    //in raw mode: we just dont open the device
    if (target == Manager::Raw) return true;
    return (**device).open(mode);
}

bool Manager::start() {
    say("Starting manager...");

    if (!prepare(QIODevice::WriteOnly,&devOut)) {
        emit(errors("failed to  start output"));
        emit(stoped());
    }
    else if (!prepare(QIODevice::ReadOnly,&devIn)) {
        emit(errors("failed to start input"));
        emit(stoped());
    }
    else {
        emit(say("devices ok"));
        say("started");
        if (config.bufferSize) {
            say("creating buffer size for: " + Size::getWsize(config.bufferMaxSize));
            buffer = new CircularBuffer(config.bufferMaxSize,"manager main buffer",this);
        }
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
#ifdef MULTIMEDIA
QStringList Manager::getDevicesNames(QAudio::Mode mode) {
    return NativeAudio::getDevicesNames(mode);
}
#endif

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
        say("transfer: stoping");
        qDebug() << "devOut: " << devOut;
        if (!devOut) say("transfer: devOut is null");
        else if (!devOut->isOpen()) say("manager: transfer: devOut is closed");

        qDebug() << "devIn: " << devIn;
        if (!devIn) say("transfer: devIn is null");
        else if (!devIn->isOpen()) say("transfer: devIn is closed");

        say("manager: stoping record");
        stop();
        return;
    }
    QByteArray data = devIn->readAll();
    //QByteArray data = devIn->read(devIn->bytesAvailable());
    if (!data.size()) {
#ifdef DEBUG
        say("warning: empty read.");
#endif
        return;
    }

    bytesCount += data.size();
    //in case of no buffer usage we dont copy data to buffer (better performance)
    if (!config.bufferSize) {
#ifdef DEBUG
        if (devOut->write(data) != data.size()) {
            say("warning: the output has not played all available sound.");
        }
#else
        devOut->write(data);
#endif
    }
    else {
        //if the buffer size is too big: we just drop the datas to prevent memory overflow by this buffer
        const int available = buffer->getAvailableBytesCount() + data.size();
        buffer->append(data);
        if (available >= config.bufferSize) {
            devOut->write(buffer->getCurrentPosData(available));
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
    map[Manager::PulseAudio] = "pulse";
#endif
#ifdef PULSEASYNC
    map[Manager::PulseAudioAsync] = "pulseasync";
#endif
    map[Manager::Tcp] = "tcp";
    map[Manager::Udp] = "udp";
    map[Manager::Raw] = "raw";
    map[Manager::Pipe] = "pipe";
    map[Manager::FreqGen] = "freqgen";
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
