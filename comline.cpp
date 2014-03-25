#include "comline.h"
#include "manager.h"
#include <QTextStream>
#include <QAudioDeviceInfo>

Comline::Comline(QObject *parent) :
    QObject(parent)
{
    QTextStream out(stdout);
    out << "comline mode" << endl;
    QAudioDeviceInfo info = QAudioDeviceInfo::availableDevices(QAudio::AudioInput).at(0);
    Manager::userConfig mc;
    mc.bufferSize = 75;
    mc.bufferMaxSize = 2*1024*1024; //2Mb
    mc.channels = 2;
    mc.codec = "audio/pcm";
    mc.devices.input = 0;
    mc.modeInput = Manager::Device;
    mc.modeOutput = Manager::Tcp;
    mc.tcpTarget.host = "192.169.1.1";
    mc.tcpTarget.port = 1042;
    mc.sampleRate = info.supportedSampleRates().last();
    mc.sampleSize = info.supportedSampleSizes().last();

    manager = new Manager(this);
    manager->setUserConfig(mc);

}
void Comline::start() {
    manager->start();
}
