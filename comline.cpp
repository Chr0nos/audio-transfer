#include "comline.h"
#include "manager.h"
#include <QTextStream>

Comline::Comline(QObject *parent) :
    QObject(parent)
{
    QTextStream out(stdout);
    out << "comline mode" << endl;

    Manager::userConfig mc;
    mc.bufferSize = 75;
    mc.channels = 2;
    mc.codec = "audio/pcm";
    mc.devices.input = 0;
    mc.modeInput = Manager::Device;
    mc.modeOutput = Manager::Tcp;
    mc.tcpTarget.host = "192.169.1.1";
    mc.tcpTarget.port = 1042;
    mc.sampleRate = 44100;
    mc.sampleSize = 16;

    manager = new Manager(this);
    manager->setUserConfig(mc);

}
void Comline::start() {
    manager->start();
}
