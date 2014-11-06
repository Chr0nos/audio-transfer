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
    bDebug = true;
    quiet = false;
    lastReadedValue = 0;
    ini = new Readini(MainWindow::getConfigFilePath(),this);
    timer = new QTimer(this);
    timer->setInterval(5000);
    connect(timer,SIGNAL(timeout()),this,SLOT(showStats()));

    out = new QTextStream(stdout);

    //preparing config default values
    initResult = initConfig();

    //parsing user arguments
    parse(argList);
}
Comline::~Comline() {
    say("exiting");
}

bool Comline::start() {
    manager = new Manager(this);
    connect(manager,SIGNAL(stoped()),this,SLOT(sockClose()));
    connect(manager,SIGNAL(debug(QString)),this,SLOT(debug(QString)));
    connect(manager,SIGNAL(errors(QString)),this,SLOT(debug(QString)));
    manager->setUserConfig(mc);
    say("format specifications: ");
    say("sample rate: " + QString::number(mc.format->getSampleRate()));
    say("sample size: " + QString::number(mc.format->getSampleSize()));
    say("channels: " + QString::number(mc.format->getChannelsCount()));
    say("codec: " + mc.format->getCodec());

    if (manager->start()) {
        timer->start();
        return true;
    }
    else say("failed to start manager");
    return false;
}
void Comline::debug(QString message) {
    if (bDebug) say(message);
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
    mc.format->setCodec("audio/pcm");
    mc.format->setSampleRate(44100);
    mc.format->setSampleSize(16);
    mc.format->setChannelCount(2);

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
    mc.tcpTarget.port = 1042;
    loadIni();

    mc.tcpTarget.sendConfig = true;

#ifdef PULSE
    mc.modeInput = Manager::PulseAudio;
#else
    mc.modeInput = Manager::Device;
#endif
    return true;
}

void Comline::parse(QStringList *argList) {
    if ((argList->contains("--help")) || (argList->contains("-h"))) {
        *out << "availables arguments:" << endl
                << "-c <x> : x is the number of channels to send" << endl
                << "-i <mode> : use this input mode" << endl
                << "-o <mode> : this this output mode" << endl
                << "\tavailables modes are:" << endl
                #ifdef PULSE
                << "\t- pulse (default input) : pulseaudio api" << endl
                #endif
                #ifdef MULTIMEDIA
                << "\t- native" << endl
                 #endif
                #ifdef PORTAUDIO
                << "\t- portaudio" << endl
                #endif
                << "\t- zero" << endl
                << "\t- tcp:<host>[:port]" << endl
                << "\t- udp:<host>[:port]" << endl
                << "\t- file:<filePath>" << endl
                << "\t- pipe" << endl
                << "\tend of availables modes" << endl
                << "-t <interval (msecs)> : set the interval between each speed refresh" << endl
                << "-n <filePath> : load the specified ini config file path" << endl
                << "-f <freq> : set the 'freq' as format frequency (default: 44100)" << endl
                << "-d : turn on debug mode" << endl
                << "end of help" << endl;
        exit(0);
    }
    else if (argList->contains("--test-circular")) {
        debug("runing circular buffer test");
        circularTest();
        exit(0);
    }
    else if (argList->contains("--version")) {
        debug("version: 1.0-git");
        exit(0);
    }

    const int m = argList->count();
    for (int i = 0; i < m ; i++) {
        QString arg = argList->at(i);

        if (arg.isEmpty()) continue;
        else if (arg == "--no-gui") continue;
        else if ((arg == "--quiet") || (arg == "-q")) {
            quiet = true;
            bDebug = false;
        }
        else if (arg == "-d") {
            say("debug mode turned on !");
            bDebug = true;
        }
        //over this line: the option NEED an argument.
        else if (i++ < m) {
            QString value = argList->at(i);
            if (value.mid(0,2) == "--") {
                say("invalid value for argument: " + arg + " -> " + value);
                exit(1);
            }
            else if ((arg == "--channels") || (arg == "-c")) {
                say("setting channels number to: " + value);
                mc.format->setChannelCount(value.toInt());
            }
            else if ((arg == "-o") || (arg == "-i")) {
                QStringList raw = value.split(":");
                if (raw.count() > 1) {
                    value = raw.first();
                    raw.removeFirst();
                }
                Manager::Mode mode = stringToMode(&value);
                if (mode == Manager::None) {
                    debug("unknow string mode: " + value);
                    exit(1);
                }
                else if (mode == Manager::File) {
                    if ((raw.count() == 1) && (raw.first() != "file")) {
                        //todo : fix this bug : detect is it's for input or output (this will be a hudge sh*t !)ls
                        say("using file path: " + raw.last());
                        mc.filePathInput = raw.last();
                        mc.filePathOutput = raw.last();
                    }
                    else {
                        say("require the file path as extra argument: file:<filePath>");
                        exit(1);
                    }
                }
                if (arg == "-o") {
                    mc.modeOutput = mode;
                    if ((mode == Manager::Tcp) || (mode == Manager::Udp)) {
                        //setting host at first argument
                        if (raw.count() >= 1) mc.tcpTarget.host = raw.first();

                        //if there is a second argument, we assume it's the port number
                        if ((raw.count() > 1) && (mc.tcpTarget.port = raw.at(1).toInt())) mc.tcpTarget.port = raw.at(1).toInt();
                    }
                }
                else mc.modeInput = mode;
            }
            else if (arg == "-n") {
                debug("setting ini file path to: " + value);
                iniPath = value;
            }
            else if (arg == "-b") {
                if (value.right(1) != "b") value = value + "b";
                mc.bufferSize = Size::getRsize(value);
                say("buffer size has been set to " + QString::number(mc.bufferSize));
            }
            else if (arg == "-t") {
                const int time = value.toInt();
                if (time) timer->setInterval(time);
                else if (timer) {
                    disconnect(timer,SIGNAL(timeout()),this,SLOT(showStats()));
                    timer->stop();
                    timer->deleteLater();
                    timer = NULL;
                }
            }
            else if (arg == "-f") {
                if (!value.toInt()) {
                    say("error: invalid frequency: " + value);
                    exit(1);
                }
                mc.format->setSampleRate(value.toInt());
            }
            else say("unknow argument: " + arg);
        }
        else say("option " + arg + " passed but argument is missing !");

    }
    start();
}
Manager::Mode Comline::stringToMode(const QString *name) {
#ifdef MULTIMEDIA
    if (*name == "native") return Manager::Device;
#endif
#ifdef PULSE
   else if (*name == "pulse") return Manager::PulseAudio;
#endif
#ifdef PORTAUDIO
    else if (*name == "portaudio") return Manager::PortAudio;
#endif
    else if (*name == "tcp") return Manager::Tcp;
    else if (*name == "udp") return Manager::Udp;
    else if (*name == "zero") return Manager::Zero;
    else if (*name == "pipe") return Manager::Pipe;
    else if (*name == "file") return Manager::File;
    return Manager::None;
}
void Comline::say(const QString message) {
    if (!quiet) *out << message << endl;
}
void Comline::loadIni() {
    if ((ini) && (ini->exists())) {
        //say("loading ini config file: " + ini->getFilePath());
        QStringList tcpInfo = ini->getValue("target","tcp").split(":");
        if (tcpInfo.count() == 2) {
            mc.tcpTarget.host = tcpInfo.at(0);
            mc.tcpTarget.port = tcpInfo.at(1).toInt();
        }

        mc.format->setCodec(ini->getValue("format","codec"));
        mc.format->setSampleRate(ini->getValue("format","sampleRate").toInt());
        mc.format->setSampleSize(ini->getValue("format","sampleSize").toInt());
        mc.format->setChannelCount(ini->getValue("format","channels").toInt());
    }

}
