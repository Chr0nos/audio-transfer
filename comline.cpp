#include "comline.h"
#include "manager.h"
#include "readini.h"
#include "mainwindow.h"
#include "size.h"
#include "circularbuffer.h"

#include <QTextStream>
#include <QAudioDeviceInfo>

Comline::Comline(QStringList *argList, QObject *parent) :
    QObject(parent)
{
    (void) argList;
    lastReadedValue = 0;
    ini = new Readini(MainWindow::getConfigFilePath(),this);
    timer = new QTimer(this);
    timer->setInterval(5000);
    connect(timer,SIGNAL(timeout()),this,SLOT(showStats()));

    out = new QTextStream(stdout);
    *out << "comline mode" << endl;

    //preparing config default values
    initResult = initConfig();

    //parsing user arguments
    parse(argList);
}
bool Comline::start() {
    if (!ini->exists()) {
        debug("ini file dosent exists");
        return false;
    }
    else if (!initResult) {
        debug("init failed");
        return false;
    }

    manager = new Manager(this);
    connect(manager,SIGNAL(stoped()),this,SLOT(sockClose()));
    connect(manager,SIGNAL(debug(QString)),this,SLOT(debug(QString)));
    connect(manager,SIGNAL(errors(QString)),this,SLOT(debug(QString)));
    manager->setUserConfig(mc);

    if (manager->start()) {
        timer->start();
        return true;
    }
    else *out << "failed to start manager" << endl;
    return false;
}
void Comline::debug(QString message) {
    *out << message << endl;
}
void Comline::showStats() {
    quint64 size = manager->getTransferedSize();
    int speed = (size - lastReadedValue) / timer->interval() * 1000;
    *out << "transfered data: " << Size::getWsize(size)  << " speed: " << Size::getWsize(speed) << "/s needed average: " << Size::getWsize(mc.format->getBytesSizeForDuration(1000)) << "/s" << endl;
    lastReadedValue = size;
}
void Comline::sockClose() {
    timer->stop();
    debug("stoped");
    exit(1);
}
void Comline::circularTest() {
    CircularBuffer::runTest();
}
bool Comline::initConfig() {
    mc.format = new AudioFormat();
    mc.format->setCodec(ini->getValue("format","codec"));
    mc.format->setSampleRate(ini->getValue("format","sampleRate").toInt());
    mc.format->setSampleSize(ini->getValue("format","sampleSize").toInt());
    mc.format->setChannelCount(ini->getValue("format","channels").toInt());

    QAudioDeviceInfo info = QAudioDeviceInfo::availableDevices(QAudio::AudioInput).at(0);
    mc.bufferSize = 0;
    mc.bufferMaxSize = 2*1024*1024; //2Mb


    mc.devices.input = 0;
    mc.devices.output = 0;
    mc.modeInput = Manager::Device;
#ifdef PULSE
    mc.modeInput = Manager::PulseAudio;
#endif
    mc.modeOutput = Manager::Tcp;

    QStringList tcpInfo = ini->getValue("target","tcp").split(":");
    mc.tcpTarget.port = 1042;

    if (tcpInfo.count() == 2) {
        mc.tcpTarget.host = tcpInfo.at(0);
        mc.tcpTarget.port = tcpInfo.at(1).toInt();
    }
    mc.tcpTarget.sendConfig = true;

    mc.modeInput = Manager::Zero;
    return true;
}

void Comline::parse(QStringList *argList) {
    if (argList->contains("--help")) {
        *out << "availables arguments:" << endl << "--host <target host> : set the target to specified host" << endl
                << "--channels <x> : x is the number of channels to send" << endl
                << "--no-gui : dont use the user interface" << endl
                << "--no-stats : dont show speeds/data transfer stats" << endl;
        exit(0);
    }
    else if (argList->contains("--test-circular")) {
        debug("runing circular buffer test");
        circularTest();
        exit(0);
    }

    const int m = argList->count();
    for (int i = 0; i < m ; i++) {
        QString arg = argList->at(i);

        if (arg.isEmpty()) continue;
        else if (arg == "--no-gui") continue;
        else if (arg == "--no-stats") {
            disconnect(timer,SIGNAL(timeout()),this,SLOT(showStats()));
            *out << "disabled stats output" << endl;
        }
        else if (i++ < m) {
            QString value = argList->at(i);
            if (value.mid(0,2) == "--") {
                *out << "invalid value for argument: " << arg << " -> " << value << endl;
                exit(1);
            }
            if ((arg == "--host") || (arg == "-h")) {
                const QStringList raw = value.split(":");
                const QString host = raw.first();
                if (raw.count() > 1) {
                    const int port = raw.last().toInt();
                    debug("setting target port to " + QString::number(port));
                    mc.tcpTarget.port = port;
                }

                *out << "setting target host to " << host << endl;
                mc.tcpTarget.host = host;
            }
            else if ((arg == "--channels") || (arg == "-c")) {
                *out << "setting channels number to: " << value << endl;
                mc.format->setChannelCount(value.toInt());
            }
            else *out << "unknow argument: " << arg;
        }
        else *out << "option " << arg << " passed but argument is missing !" << endl;

    }
    start();
}
