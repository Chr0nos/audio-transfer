#include "manager.h"
#include "size.h"

#include <QString>
#include <QIODevice>
#include <QFile>
#include <QMap>

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

Manager::~Manager()
{
    say("deleting manager");
    if (devIn) delete(devIn);
    if (devOut) delete(devOut);
    if (buffer) delete(buffer);
    delete(format);
}

//pointer of function
typedef ModuleDevice* (*FactoryFct_t)(QString, AudioFormat*, void*, QObject*);

bool Manager::prepare(QIODevice::OpenModeFlag mode, QIODevice **device)
{

    ModuleDevice    *dev;
    QString         name;
    QString         *filePath;
    Manager::Mode   target;
    QIODevice       *rawDev;
    int             deviceId;
    QMap<Manager::Mode, FactoryFct_t> devptr;

    if ((device) && ((*device)))
    {
        (**device).disconnect();
        delete(*device);
    }
    dev = NULL;
    rawDev = NULL;
    *device = NULL;
    target = Manager::None;
    filePath = NULL;
    deviceId = 0;

    if (mode == QIODevice::ReadOnly)
    {
        target = config.modeInput;
        deviceId = config.devices.input;
        filePath = &config.file.input;
        rawDev = config.raw.devIn;
        if (!config.devicesNames.input.isEmpty()) name = config.devicesNames.input;
    }
    else if (mode == QIODevice::WriteOnly)
    {
        target = config.modeOutput;
        deviceId = config.devices.output;
        filePath = &config.file.output;
        rawDev = config.raw.devOut;
        if (!config.devicesNames.output.isEmpty()) name = config.devicesNames.output;
    }
#ifdef MULTIMEDIA
    devptr[Manager::Device] = &(NativeAudio::factory);
#endif
#ifdef ASIO
    devptr[Manager::AsIO] = &(AsioDevice::factory);
#endif
#ifdef PULSE
    devptr[Manager::PulseAudio] = &(PulseDevice::factory);
#endif
#ifdef PULSEASYNC
    devptr[Manager::PulseAudioAsync] = &(PulseDeviceASync::factory);
#endif
#ifdef PORTAUDIO
    devptr[Manager::PortAudio] = &(PortAudioDevice::factory);
#endif
    devptr[Manager::Tcp] = &(TcpDevice::factory);
    devptr[Manager::Udp] = &(UdpDevice::factory);
    devptr[Manager::Zero] = &(ZeroDevice::factory);
    devptr[Manager::Pipe] = &(PipeDevice::factory);
    devptr[Manager::FreqGen] = &(Freqgen::factory);

    if (devptr.contains(target))
    {
        dev = devptr[target](name, format,&this->config ,this);
        dev->setDeviceId(mode, deviceId);
    }

    else if (target == Manager::File)
    {
        *device = new QFile(*filePath);
    }
    else if (target == Manager::Raw)
    {
        *device = rawDev;
    }

    if (dev)
    {
        connect(dev, SIGNAL(debug(QString)), this, SIGNAL(debug(QString)));
        *device = dev;
    }
    if (!*device) return false;
    if (mode == QIODevice::ReadOnly) connect(*device,SIGNAL(readyRead()),this,SLOT(transfer()));

    //if a name is available lets assign it to the new device
    if (!name.isEmpty()) (**device).setObjectName(name);

    //in raw mode: we just dont open the device
    if (target == Manager::Raw) return true;
    return (**device).open(mode);
}

bool Manager::start()
{
    say("Starting manager...");

    if (!prepare(QIODevice::WriteOnly,&devOut))
    {
        emit(errors("failed to  start output"));
        emit(stoped());
    }
    else if (!prepare(QIODevice::ReadOnly,&devIn))
    {
        emit(errors("failed to start input"));
        emit(stoped());
    }
    else {
        emit(say("devices ok"));
        say("started");
        if (config.bufferSize)
        {
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

void Manager::stop()
{
    bisRecording = false;
    if (devIn)
    {
        devIn->close();
        disconnect(devIn,SIGNAL(readyRead()),this,SLOT(transfer()));
        delete(devIn);
        devIn = NULL;
    }
    if (devOut)
    {
        devOut->close();
        delete(devOut);
        devOut = NULL;
    }
    emit(stoped());
}

#ifdef MULTIMEDIA
QStringList Manager::getDevicesNames(QAudio::Mode mode)
{
    return NativeAudio::getDevicesNames(mode);
}
#endif

void Manager::setUserConfig(userConfig cfg)
{
    if (isRecording())
    {
        emit(errors("can't change user configuration while transfering."));
        return;
    }
    config = cfg;
    format = cfg.format;
}

bool Manager::transferChecks()
{
    /*
    ** this function return true if the transfer is ready to go
    ** otherwise it will return false and cause the stop of record
    */
    //qDebug() << "transfer!" << devIn << devIn->isOpen();
    if ((!devOut) || (!devIn) || (!devIn->isOpen()) || (!devOut->isOpen()))
    {
        say("transfer: stoping");
        qDebug() << "devOut: " << devOut;
        if (!devOut) say("transfer: devOut is null");
        else if (!devOut->isOpen()) say("manager: transfer: devOut is closed");

        qDebug() << "devIn: " << devIn;
        if (!devIn) say("transfer: devIn is null");
        else if (!devIn->isOpen()) say("transfer: devIn is closed");

        say("manager: stoping record");
        stop();
        return false;
    }
    return true;
}

void Manager::transfer()
{
    /*
    ** this slot is LITERALY the core of the program
    ** it read the sound from a module (QIODevice*) every time a
    ** "readyRead" signal is emited by the input module
    ** and write it imediatly to the target
    ** the "dev->write means "play that sound"
    */
    if (!transferChecks()) return;

    QByteArray data = devIn->readAll();
    //QByteArray data = devIn->read(devIn->bytesAvailable());
    if (!data.size())
    {
    #ifdef DEBUG
        say("warning: empty read.");
    #endif
        return;
    }

    bytesCount += data.size();
    //in case of no buffer usage we dont copy data to buffer (better performance)
    if (!config.bufferSize)
    {
        devOut->write(data);
    }
    else
    {
        //if the buffer size is too big: we just drop the datas to prevent memory overflow by this buffer
        const int available = buffer->getAvailableBytesCount() + data.size();
        buffer->append(data);
        if (available >= config.bufferSize)
        {
            devOut->write(buffer->getCurrentPosData(available));
        }
    }
}

quint64 Manager::getTransferedSize()
{
    return bytesCount;
}

bool Manager::isRecording()
{
    return bisRecording;
}

QStringList Manager::intListToQStringList(QList<int> source)
{
    QStringList result;
    QList<int>::iterator i;
    for (i = source.begin();i != source.end();i++) {
        result << QString::number(*i);
    }
    return result;
}

void Manager::debugList(const QStringList list)
{
    int count = 0;
    foreach (QString x,list) {
        debug("[" + QString::number(count++) + "] " + x);
    }
}

void Manager::devOutClose()
{
    say("output closed");
    stop();
}

void Manager::devInClose()
{
    say("input closed");
    stop();
}

void Manager::say(const QString message)
{
    emit(debug("Manager: " + message));
}

QMap<Manager::Mode,QString> Manager::getModesMap()
{
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
#ifdef ASIO
    map[Manager::AsIO] = "ASIO";
#endif
    map[Manager::Tcp] = "tcp";
    map[Manager::Udp] = "udp";
    map[Manager::Raw] = "raw";
    map[Manager::Pipe] = "pipe";
    map[Manager::FreqGen] = "freqgen";
    return map;
}

Manager::Mode Manager::getModeFromString(const QString *name)
{
    QMap<Manager::Mode,QString>::iterator i;
    QMap<Manager::Mode,QString> map = Manager::getModesMap();
    for (i = map.begin() ; i != map.end() ; i++)
    {
        if (*name == i.value()) return i.key();
    }
    return Manager::None;
}

QString Manager::getStringFromMode(const Manager::Mode *mode)
{
    QMap<Manager::Mode,QString> map = getModesMap();
    QMap<Manager::Mode,QString>::iterator i;
    for (i = map.begin() ; i != map.end() ; i++)
    {
        if (i.key() == *mode) return i.value();
    }
    return QString("none");
}
