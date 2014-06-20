#include "comline.h"
#include "manager.h"
#include <QTextStream>
#include <QAudioDeviceInfo>

Comline::Comline(QObject *parent) :
    QObject(parent)
{
    out = new QTextStream(stdout);
    *out << "comline mode" << endl;


    QAudioDeviceInfo info = QAudioDeviceInfo::availableDevices(QAudio::AudioInput).at(0);
    Manager::userConfig mc;
    mc.bufferSize = 0;
    mc.bufferMaxSize = 2*1024*1024; //2Mb
    mc.format = new AudioFormat();
    mc.format->setCodec("audio/pcm");
    mc.format->setSampleRate(info.supportedSampleRates().last());
    mc.format->setSampleSize(info.supportedSampleSizes().last());
    mc.format->setChannelCount(2);

    mc.devices.input = 0;
    mc.devices.output = 0;
    mc.modeInput = Manager::Device;
    mc.modeOutput = Manager::Tcp;
    mc.tcpTarget.host = "127.0.0.1";
    mc.tcpTarget.port = 1042;
    mc.tcpTarget.sendConfig = true;

    manager = new Manager(this);
    connect(manager,SIGNAL(debug(QString)),this,SLOT(debug(QString)));
    connect(manager,SIGNAL(errors(QString)),this,SLOT(debug(QString)));
    manager->setUserConfig(mc);
}
void Comline::start() {
    manager->start();
}
void Comline::debug(QString message) {
    *out << message << endl;
}
