#include "qrec.h"

#include <QUrl>
#include <QDebug>
#include <QFile>
#include <QTimer>
#include <QDataStream>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QTcpServer>
#include <QtMultimedia/QAudioOutput>
#include <QtMultimedia/QAudioInput>
#include <QtMultimedia/QAudioDeviceInfo>

QRec::QRec(QObject *parent) :
    QObject(parent)
{
    config.codec = "audio/pcm";
    config.sampleRate = 44100;
    config.channels = 2;
    config.sampleSize = 8;
    readedData = 0;
    bIsRecording = false;
    devIn = 0;
    devOut = 0;
    sourceFile = 0;
    audioInput = 0;
    tcpSourceServ = 0;
    //audio/pcm = 1500bps
    tcpOutputBufferSize = 75; //50ms

    //by default: i put the modes to undefined, the user will define it while asking for a source or a target
    currentSourceMode = this->Undefined;
    currentTargetMode = this->Undefined;
    currentSourceInfo = QAudioDeviceInfo::defaultInputDevice();

    //defining the first output device found as the source by default
    this->source = this->getDevicesList(QAudio::AudioOutput).first();
}

QStringList QRec::getDevicesList(QAudio::Mode type) {
    QStringList devices;
    foreach (const QAudioDeviceInfo &deviceInfo, QAudioDeviceInfo::availableDevices(type)) {
        devices << deviceInfo.deviceName();
    }
    return devices;
}
QStringList QRec::getDevicesList() {
    QStringList names;
    QList<QAudioDeviceInfo> devices = this->getAllAudioDevice();
    QList<QAudioDeviceInfo>::iterator i;
    for (i = devices.begin();i != devices.end();i++) {
        QAudioDeviceInfo &x = *i;
        names << x.deviceName();
    }
    return QStringList();
}

void QRec::listDevices() {
    //this function just list devices by names
    int count = 0;
    qDebug() << "listing devices:";
    foreach (QString device, this->getDevicesList()) {
        qDebug() << count++ << ": Device name: " << device;
    }
}
void QRec::redirectFlushed() {
    qDebug() << "Flushed !";
}
void QRec::volumeChanged(qreal volume) {
    qDebug() << "Volume changed: " + QString().number(volume);
}
void QRec::deviceChanged(QString newDevice) {
    qDebug() << "Device changed, new device: " + newDevice;
}
void QRec::setTargetFilePath(const QString filePath) {
    targetFile.setFileName(filePath);
    currentTargetMode = QRec::File;
}
bool QRec::isRecording() {
    return bIsRecording;
}
bool QRec::setSourceId(const int deviceId) {
    QStringList devices = this->getDevicesList(QAudio::AudioOutput);
    qDebug() << devices;
    if (devices.isEmpty()) return false;
    if (deviceId >= devices.count()) return false;
    source = devices[deviceId];
    qDebug() << "new source: " + source << deviceId << "of: " << devices.count() -1;
    if (sourceFile != 0) {
        delete(sourceFile);
        sourceFile = 0;
    }
    currentSourceInfo = this->getAllAudioDevice().at(deviceId);

    this->currentSourceMode = QRec::Device;
    return true;
}
void QRec::stopRec() {
    //if current target mode is a File and opened: flusing and closing it
    if ((currentTargetMode == QRec::File) && (targetFile.isOpen())) {
        targetFile.flush();
        targetFile.close();
    }
    if (currentTargetMode == QRec::Tcp) {
        if (tcp) {
            tcp->disconnectFromHost();
            tcp->deleteLater();
        }
        tcp = 0;
        devOut = 0;
    }

    //if a devIn is defined (device input):
    if (devIn) {
        //close the source listener
        if (audioInput != 0) audioInput->stop();
        //close the QIODevice
        //devIn->close();
        devIn = 0;
        if (currentSourceMode == QRec::Tcp) {
            if (tcpSourceServ != 0) {
                tcpSourceServ->close();
                devIn = 0;
            }
            if (tcpSource != 0) {
                if (tcpSource->isOpen()) tcpSource->close();
            }
        }
    }
    bIsRecording = false;
    //if (audioInput) delete(audioInput);
    qDebug() << "Recording stoped" << "readed data: " << readedData;
    emit(stoped());
}
QStringList QRec::getSupportedCodec() {
    return currentSourceInfo.supportedCodecs();
}
QStringList QRec::getSupportedSamplesRates() {
    QStringList rates;
    foreach (int rate,currentSourceInfo.supportedSampleRates()) {
        rates << QString::number(rate);
    }
    return rates;
}
QStringList QRec::getSupportedSamplesSizes() {
    QStringList sizes;
    foreach (int size,currentSourceInfo.supportedSampleSizes()) {
        sizes << QString::number(size);
    }
    return sizes;
}

void QRec::deviceStateChanged(QAudio::State state) {
    qDebug() << "Device state changed: " << state;
}
void QRec::flusher() {

}
bool QRec::startRecAlt() {
    QAudioFormat format;
    format.setChannelCount(config.channels);
    format.setSampleSize(config.sampleSize);
    format.setCodec(config.codec);
    format.setSampleRate(config.sampleRate);
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::UnSignedInt);

    QAudioDeviceInfo info = getAudioDeviceByName(source);
    qDebug() << "info ready";

    //QAudioDeviceInfo info = QAudioDeviceInfo::defaultInputDevice();

    qDebug() << "supported channels:" << info.supportedChannelCounts();
    qDebug() << "Selected device: " + info.deviceName();
    if (!info.isFormatSupported(format)) {
        qDebug() << "default format not supported try to use nearest";
        qDebug() << info.supportedChannelCounts() << info.supportedCodecs() << info.supportedSampleSizes();
        format = info.nearestFormat(format);
    }

    readedData = 0;
    //if the source is not a file
    if (sourceFile == 0) {
        audioInput = new QAudioInput(info,format,this);
        devIn = audioInput->start();
        if (!devIn) {
            qDebug() << "error: cannot start the input: " << info.deviceName() << audioInput->error();
            delete(audioInput);
            audioInput = 0;
            devIn = 0;
            return false;
        }
        connect(audioInput,SIGNAL(stateChanged(QAudio::State)),this,SLOT(deviceStateChanged(QAudio::State)));
        connect(audioInput,SIGNAL(objectNameChanged(QString)),this,SLOT(deviceChanged(QString)));
        connect(devIn,SIGNAL(readyRead()),this,SLOT(readyRead()));
    }
    //if the source is a file: open the file, and set devIn point to sourceFile opened
    else {
        qDebug() << "Setting file as source: " << sourceFile->fileName();    }

    //if target is a file
    if (currentTargetMode == QRec::File) {
        qDebug() << "target file mode: " << targetFile.fileName();
        if (targetFile.exists()) targetFile.remove();
        if (!targetFile.open(QIODevice::WriteOnly)) {
            qDebug() << "failed to open target file: " << targetFile.errorString();
            return false;
        }
        devOut = &targetFile;
    }
    else if (currentTargetMode == QRec::Device) qDebug() << "target device mode.";
    else if (currentTargetMode == QRec::Tcp) {
        qDebug() << "target tcp mode";
        devOut = tcp;
        QTextStream out(tcp);
        out << "samplerate:" << config.sampleRate << " samplesize:" << config.sampleSize << " channels:" << config.channels << endl;
        qDebug() << "configuration sent";
    }

    if ((!devIn) || (!devOut)) {
        qDebug() << "Error no input or output: " << devIn << devOut;
        return false;
    }

    bIsRecording = true;
    if (devIn == sourceFile) capture();
    //QTimer::singleShot(3000,this,SLOT(stopRec()));
    return true;
}
void QRec::readyRead() {
    capture();
}
void QRec::capture() {
    QByteArray data = devIn->readAll();
    if (tcpOutputBufferSize) {
        tcpOutputBuffer.append(data);
        if (tcpOutputBuffer.size() >= tcpOutputBufferSize) {
            devOut->write(tcpOutputBuffer);
            //if (currentTargetMode == QRec::File) targetFile.flush();
            tcpOutputBuffer.clear();
        }
    }
    else devOut->write(data);

    readedData += data.size();
    //qDebug() << data.toHex();
    //qDebug() << "readed: " << readedData;
}

QAudioDeviceInfo QRec::getAudioDeviceById(const int id,QAudio::Mode type = QAudio::AudioOutput) {
    QList<QAudioDeviceInfo> devices;
    foreach (const QAudioDeviceInfo &deviceInfo, QAudioDeviceInfo::availableDevices(type)) {
        devices << deviceInfo;
    }
    return devices.at(id);
}
QAudioDeviceInfo QRec::getAudioDeviceById(const int id) {
    return getAllAudioDevice().at(id);
}

QList<QAudioDeviceInfo> QRec::getAllAudioDevice() {
    //this method return all input and output devices availables on the local computer
    QList<QAudioDeviceInfo> devices;
    QList<QAudio::Mode> states;
    states << QAudio::AudioInput;
    //states << QAudio::AudioOutput;
    foreach (QAudio::Mode currentState,states) {
        foreach (const QAudioDeviceInfo deviceInfo, QAudioDeviceInfo::availableDevices(currentState)) {
            devices.append(deviceInfo);
        }
    }
    return devices;
}
QStringList QRec::getAllAudioDevicesNames() {
    QStringList devicesNames;
    foreach (const QAudioDeviceInfo &deviceInfo,this->getAllAudioDevice()) {
        devicesNames << deviceInfo.deviceName();
    }
    return devicesNames;
}

QAudioDeviceInfo QRec::getAudioDeviceByName(const QString name) {
    QList<QAudioDeviceInfo> devices = this->getAllAudioDevice();
    QStringList devicesNames = this->getAllAudioDevicesNames();
    const int pos = devicesNames.indexOf(name);
    if (pos < 0) return QAudioDeviceInfo::defaultInputDevice();
    if (pos > devices.count()) return devices.first();
    return devices.at(pos);
}
void QRec::setAudioOutput(QIODevice *output) {
    devOut = output;
    this->currentTargetMode = QRec::Device;
}
void QRec::setSourceFilePath(const QString filePath) {
    //segfault
    //if (sourceFile) delete(sourceFile);
    sourceFile = new QFile(filePath);
    if (!sourceFile->exists()) sourceFile = 0;
    else {
        devIn = sourceFile;
        //if the output is a file: no need an output buffer.
        this->tcpOutputBufferSize = 0;
        this->currentSourceMode = QRec::File;
    }
}
bool QRec::setTargetTcp(const QString host, const int port) {
    if ((!port) || (port > 65534)) return false;
    tcp = new QTcpSocket;
    tcp->setSocketOption(QAbstractSocket::LowDelayOption,0);
    tcp->connectToHost(host,port);
    tcp->waitForConnected();
    devOut = tcp;
    this->currentTargetMode = QRec::Tcp;
    connect(tcp,SIGNAL(disconnected()),this,SLOT(tcpDisconnect()));
    connect(tcp,SIGNAL(connected()),this,SLOT(tcpTargetConnected()));
    return true;
}
void QRec::tcpDisconnect() {
    qDebug() << "Tcp disconected: stopping record";
    this->stopRec();
}
bool QRec::setSourceTcp(const QString authorisedHosts, const int port) {
    //just to prevent the warning...
    if (authorisedHosts.isNull()) {}
    tcpSourceServ = new QTcpServer;
    tcpSourceServ->listen(QHostAddress::Any,port);
    connect(tcpSourceServ,SIGNAL(newConnection()),this,SLOT(tcpNewConnection()));
    return true;
}
void QRec::tcpNewConnection() {
    tcpSource = tcpSourceServ->nextPendingConnection();
    tcpSource->waitForConnected();
    connect(tcpSource,SIGNAL(disconnected()),this,SLOT(tcpSourceDisconnected()));
    connect(tcpSource,SIGNAL(readyRead()),this,SLOT(readyRead()));
    currentSourceMode = QRec::Tcp;
    devIn = tcpSource;
    qDebug() << "new tcp connection from " << tcpSource->peerAddress().toString();
}

void QRec::tcpSourceDisconnected() {
    disconnect(tcpSource,SIGNAL(readyRead()),this,SLOT(readyRead()));
    disconnect(tcpSource,SIGNAL(disconnected()),this,SLOT(tcpSourceDisconnected()));
    qDebug() << "tcp source: disconnected: " << tcpSource->errorString();
    currentSourceMode = QRec::Undefined;
    this->stopRec();
}
int QRec::getMaxChannelsCount() {
    return currentSourceInfo.supportedChannelCounts().last();
}
void QRec::setTcpOutputBuffer(const int bufferTimeInMs) {
    if (!bufferTimeInMs) this->tcpOutputBufferSize = 0;
    else this->tcpOutputBufferSize = (int) 1500 / (bufferTimeInMs / 1000);
}
void QRec::tcpTargetConnected() {
    emit(targetConnected());
}
void QRec::setUserConfig(userConfig config) {
    this->config = config;
}
quint64 QRec::getReadedData() {
    return readedData;
}
