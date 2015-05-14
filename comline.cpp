#include "comline.h"
#include "manager.h"
#include "readini.h"
#ifdef GUI
#include "ui/mainwindow.h"
#endif
#include "size.h"
#include "circularbuffer.h"
#ifdef SERVER
#include "server/serversocket.h"
#endif
#ifdef MULTIMEDIA
#include <QAudioDeviceInfo>
#endif

#include <QTextStream>
#include <QTime>
#include <QDir>
#include <QCoreApplication>

Comline::Comline(QStringList *argList, QObject *parent) :
    QObject(parent)
{
    bDebug = true;
    quiet = false;
    sayUseTime = false;
    lastReadedValue = 0;
    QString cfgPath;
#ifdef GUI
    cfgPath = MainWindow::getConfigFilePath();
#else
    cfgPath = QDir::homePath() + "/.audio-transfer/client.ini";
#endif
    ini = new Readini(cfgPath,this);

    //this is the transfer stats refresh timer
    timer = new QTimer(this);
    timer->setInterval(5000);
    connect(timer,SIGNAL(timeout()),this,SLOT(showStats()));

    out = new QTextStream(stdout);

    //preparing config default values
    initResult = initConfig();

    this->serverType = ServerSocket::Invalid;

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
        if (timer) timer->start();
        return true;
    }
    else say("failed to start manager");
    return false;
}
void Comline::debug(QString message) {
    if (bDebug) say(message);
}
void Comline::showStats() {
    if (!manager) return;
    quint64 size = manager->getTransferedSize();
    int speed = (size - lastReadedValue) / timer->interval() * 1000;
    say("transfered data: " + Size::getWsize(size)  + " speed: " +
        Size::getWsize(speed) + "/s needed average: " +
        Size::getWsize(mc.format->getBytesSizeForDuration(1000)) + "/s");
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

    mc.bufferSize = 0;
    mc.bufferMaxSize = 2*1024*1024; //2Mb


    mc.devices.input = 0;
    mc.devices.output = 0;
    mc.modeInput = Manager::Device;
#ifdef PULSE
    mc.modeInput = Manager::PulseAudio;
#endif
#ifdef PORTAUDIO
    mc.portAudio.deviceIdInput = 0;
    mc.portAudio.deviceIdOutput = 0;
#endif
    mc.modeOutput = Manager::Tcp;
    mc.network.port = 1042;
    loadIni();

    mc.network.sendConfig = true;

#ifdef PULSE
    mc.modeInput = Manager::PulseAudio;
#else
    mc.modeInput = Manager::Device;
#endif
    return true;
}
void Comline::showHelp()
{
    *out << "availables arguments:" << endl
            << "-c <x> : x is the number of channels to send" << endl
            << "-i <mode> : use this input mode" << endl
            << "-o <mode> : this this output mode" << endl
            << "\tavailables modes are:" << endl
        #ifdef PULSE
            << "\t- pulse (default input)" << endl
        #endif
        #ifdef PULSEASYNC
            << "\t- pulseasync" << endl
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
            << "\t- freqgen" << endl
            << "\tend of availables modes" << endl
            << "-t <interval (msecs)> : set the interval between each speed refresh" << endl
            << "-n <filePath> : load the specified ini config file path" << endl
            << "-f <freq> : set the 'freq' as format frequency (default: 44100)" << endl
            << "-s <sample Size> : set the sample size to the value, valids samples sizes are: 8, 16, 24, 32 (depending on hardware)" << endl
            << "-d : turn on debug mode" << endl
            << "-z : enable time and time zone" << endl
            << "-h : show this help" << endl
            << "-r : show indicatives bitrates usages for any modes" << endl
       #ifdef SERVER
            << "--- For server mode only ---" << endl
            << "--server : run in server mode (input will be ignored)" << endl
            << "--server-type <type> : set type of incoming connection must be (tcp or udp) (tcp is default)" << endl
            << "--pid <filePath> : write the server process pid to the specified file" << endl
       #endif
       #ifdef DEBUG
             << "--test-cricular : run ring buffer main class self test (debug)" << endl
             << "--test-device : run ring buffer device test (debug)" << endl
             << "--hex : need to be used with -o pipe: show the output as hexadecimal (for debug purpose)" << endl
       #endif
            //<< "-platform offscreen : allow you to run the program withous any X connection" << endl
            << "end of help" << endl;
    exit(0);
}

#ifdef SERVER
void Comline::makeServer()
{
    //this thing is the server mode, it's currently in developement, the idea is to re-use the current Manager class and other stuffs
    /* this will handle:
     * - Tcp incoming connection (and handle)
     * - Udp incoming datagram
     * - Format attribution
     * - Manager initialisation for each client (one Manager object Per client) (inside a user class i guess)
     * - TimeOut detection
     *
    */
    QStringList configPath;
    QString goodConfigFilePath;

    configPath << QDir::home().path() + "/.audio-transfer/server.ini";
    configPath << QDir::home().path() + "/.audio-transfer-server.ini";
    configPath << "/etc/audio-transfer/server.ini";
    foreach (QString filePath,configPath) {
        if (QFile::exists(filePath)) {
            goodConfigFilePath = filePath;
            break;
        }
    }
    if (goodConfigFilePath.isEmpty()) {
        say("warning: no configuration file found !");
    }

    srv = new ServerMain(goodConfigFilePath,this);
    connect(srv,SIGNAL(debug(QString)),this,SLOT(debug(QString)));
}
#endif

void Comline::parse(QStringList *argList) {
    bool serverMode = false;
    if (this->serverType == ServerSocket::Invalid)
    {
        serverType = ServerSocket::Tcp;
    }
    if ((argList->contains("--help")) || (argList->contains("-h"))) showHelp();
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
    for (int i = 0 ; i < m ; i++) {
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
        else if (arg == "-z") {
            sayUseTime = true;
        }
        else if (arg == "--hex") {
            mc.pipe.hexMode = true;
        }
        else if (arg == "-r") {
            showCommonRates();
            exit(0);
        }
#ifdef SERVER
        else if (arg == "--server") {
            makeServer();
            serverMode = true;
        }
        else if (arg == "--test-device") {
            CircularDevice* circular = new CircularDevice(512,this);
            connect(circular,SIGNAL(debug(QString)),this,SLOT(debug(QString)));

            QIODevice* dev = circular;
            //connect(dev,SIGNAL(readyRead()),this,SLOT(debugTrigger()));

            dev->open(QIODevice::ReadWrite);
            qDebug() << "write: " << dev->write(QString("test data").toLocal8Bit());

            const size_t bytes = dev->bytesAvailable();
            qDebug() << "availables bytes:" << bytes;

            QByteArray data = dev->readAll();
            qDebug() << "data pointer: " << &data;
            qDebug() << "data value: " << data;

            dev->close();
            dev->deleteLater();
            exit(0);
        }
#endif
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
                Manager::Mode mode = Manager::getModeFromString(&value);
                if (mode == Manager::None) {
                    debug("unknow string mode: " + value);
                    exit(1);
                }
                else if (mode == Manager::File) {
                    if ((raw.count() == 1) && (raw.first() != "file")) {
                        //todo : fix this bug : detect is it's for input or output (this will be a hudge sh*t !)ls
                        say("using file path: " + raw.last());
                        mc.file.input = raw.last();
                        mc.file.output = raw.last();
                    }
                    else {
                        say("require the file path as extra argument: file:<filePath>");
                        exit(1);
                    }
                }
#ifdef MULTIMEDIA
                else if (mode == Manager::Device) {
                    if (!raw.count());
                    else if (raw.first() == "list") {
                        QAudio::Mode audioMode;
                        QString textMode;
                        if (arg == "-o") {
                            audioMode = QAudio::AudioOutput;
                            textMode = "output";
                        }
                        else {
                            audioMode = QAudio::AudioInput;
                            textMode = "input";
                        }

                        say(textMode + " devices list:");
                        int count = 0;
                        QStringList devices = NativeAudio::getDevicesNames(audioMode);
                        for (QStringList::iterator i = devices.begin() ; i != devices.end() ; i++) {
                            say(QString::number(count++) + ": " + *i);
                        }
                        say("end of list");
                        exit(0);
                    }
                    else {
                        mc.devices.input = raw.first().toInt();
                        say("forced device: " + QString::number(mc.devices.input));
                    }
                }
#endif
                if (arg == "-o") {
                    mc.modeOutput = mode;
                    if ((mode == Manager::Tcp) || (mode == Manager::Udp)) {
                        //setting host at first argument
                        if (raw.count() >= 1) mc.network.host = raw.first();

                        //if there is a second argument, we assume it's the port number
                        if ((raw.count() > 1) && (mc.network.port = raw.at(1).toInt())) mc.network.port = raw.at(1).toInt();

                        //allowing user to specify a sender name with: -o tcp:<address>:<port>:<senderName>
                        if (raw.count() >= 3) mc.devicesNames.output = raw.at(2);
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
                mc.bufferMaxSize = Size::getRsize(value);
                mc.bufferSize = mc.bufferMaxSize;
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
#ifdef SERVER
            else if (arg == "--server-type") {
                if (value == "tcp") serverType = ServerSocket::Tcp;
                else if (value == "udp") serverType = ServerSocket::Udp;
                else {
                    say("unknow server type parameter: " + value);
                    exit(0);
                }
            }
#endif
            else if (arg == "--pid") {
                writePid(&value);
            }

            //lets handle this by Qt itself
            else if (arg == "-platform");
            else if (arg == "-s") {
                if (!value.toInt()) {
                    say("error: wrong sample size.");
                    exit(1);
                }
                mc.format->setSampleSize(value.toInt());
            }

            else say("unknow argument: " + arg);
        }
        else say("option " + arg + " passed but argument is missing !");

    }
    if (!serverMode) start();
#ifdef SERVER
    else srv->listen(serverType);
#endif
}
void Comline::writePid(QString *value)
{
    QString &filePath = *value;
    const qint64 pid = QCoreApplication::applicationPid();
    QFile file(filePath);

    say("creating pid file to: " + filePath);
    say("current pid: " + QString::number(pid));
    if (file.exists()) file.remove();
    if (!file.open(QIODevice::WriteOnly)) {
        say("failed to create the pid file: check the permissions.");
        exit(1);
    }
    file.write(QString::number(pid).toLocal8Bit());
    file.close();
}

void Comline::say(const QString message) {
    if (!quiet) {
        if (sayUseTime) *out << QTime::currentTime().toString("hh:mm:ss t : ");
        *out << message << endl;
    }
}
void Comline::loadIni() {
    QString proto;

    if ((ini) && (ini->exists())) {
        //say("loading ini config file: " + ini->getFilePath());
        QStringList tcpInfo = ini->getValue("target","tcp").split(":");
        if (tcpInfo.count() == 2) {
            mc.network.host = tcpInfo.at(0);
            mc.network.port = tcpInfo.at(1).toInt();
        }

        mc.format->setCodec(ini->getValue("format","codec"));
        mc.format->setSampleRate(ini->getValue("format","sampleRate").toInt());
        mc.format->setSampleSize(ini->getValue("format","sampleSize").toInt());
        mc.format->setChannelCount(ini->getValue("format","channels").toInt());

        /*
        ** this part allow the protocol choice loading from the ini file
        ** it dispend user to specify "--server-type udp" in command line
        */
        proto = this->ini->getValue("general", "proto").toLower();
        if (proto == "tcp") this->serverType = ServerSocket::Tcp;
        else if (proto == "udp") this->serverType = ServerSocket::Udp;
        else this->serverType = ServerSocket::Invalid;
    }

}
void Comline::debugTrigger() {
    qDebug() << "debug trigger was called by" << sender() << qobject_cast<CircularDevice*>(sender())->bytesAvailable();
}
void Comline::showCommonRates() {
    QList<int> rates = AudioFormat::getCommonSamplesRates();
    int rate;
    int bits;
    QString rateString;

    say("indicatives bitrates usages:");
    say("Channels\tSize\tRate\t\tBitrate");
    int channels = 0;
    while (channels++ < 8) {
        bits = 8;
        while (bits <= 32) {
            foreach (rate,rates) {
                const int bitrate = rate * bits * channels / 8;
                rateString = Size::getWsize(rate,1000);
                rateString = rateString.mid(0,rateString.length() -1).rightJustified(8,QChar(32));
                say("" + QString::number(channels).rightJustified(5,QChar(32)) + "\t\t" +
                    QString::number(bits) + "bits \t" +
                    rateString + "hz\t" +
                    Size::getWsize(bitrate).rightJustified(8,QChar(32)) + "/s"
                    );
            }
            bits *= 2;
        }
    }
    say("end");
}
void Comline::showQStringList(QStringList *list) {
    QStringList::iterator i;
    for (i = list->begin() ; i != list->end() ; i++)
        *out << "-" << *i << endl;
}
