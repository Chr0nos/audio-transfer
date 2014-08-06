#include "comline.h"
#include "manager.h"
#include "readini.h"
#include "mainwindow.h"

#include <QTextStream>
#include <QAudioDeviceInfo>

Comline::Comline(QString *args, QObject *parent) :
    QObject(parent)
{
    out = new QTextStream(stdout);
    *out << "comline mode" << endl;
    *out << "arguments: " << args;
}
bool Comline::start() {
    Readini ini(MainWindow::getConfigFilePath(),this);
    if (!ini.exists()) return false;

    QAudioDeviceInfo info = QAudioDeviceInfo::availableDevices(QAudio::AudioInput).at(0);
    Manager::userConfig mc;
    mc.bufferSize = 0;
    mc.bufferMaxSize = 2*1024*1024; //2Mb
    mc.format = new AudioFormat();
    mc.format->setCodec(ini.getValue("format","codec"));
    mc.format->setSampleRate(ini.getValue("format","sampleRate").toInt());
    mc.format->setSampleSize(ini.getValue("format","sampleSize").toInt());
    mc.format->setChannelCount(ini.getValue("format","channels").toInt());

    mc.devices.input = 0;
    mc.devices.output = 0;
    mc.modeInput = Manager::Device;
#ifdef PULSE
    mc.modeInput = Manager::PulseAudio;
#endif
    mc.modeOutput = Manager::Tcp;

    QStringList tcpInfo = ini.getValue("target","tcp").split(":");
    if (!tcpInfo.count() != 2) return false;
    mc.tcpTarget.host = tcpInfo.at(0);
    mc.tcpTarget.port = tcpInfo.at(1).toInt();
    mc.tcpTarget.sendConfig = true;

    manager = new Manager(this);
    connect(manager,SIGNAL(debug(QString)),this,SLOT(debug(QString)));
    connect(manager,SIGNAL(errors(QString)),this,SLOT(debug(QString)));
    manager->setUserConfig(mc);

    return manager->start();
}
void Comline::debug(QString message) {
    *out << message << endl;
}
