#ifdef ASIO
#include "asiodevice.h"
#include "moduledevice.h"

#include "./lib/ASIOSDK2.3/common/asio.h"

AsioDevice::AsioDevice(QObject *parent) :
    ModuleDevice(parent)
{
    (void) parent;
    this->info = NULL;
}

qint64 AsioDevice::readData(char *data, qint64 maxlen)
{
    (void) data;
    (void) maxlen;
    return 0;
}

qint64 AsioDevice::writeData(const char *data, qint64 len)
{
    (void) data;
    (void) len;
    return 0;
}

bool AsioDevice::open(OpenMode mode)
{
    (void) mode;
    return true;
}

int AsioDevice::getAvailableChannels()
{
    long        input;
    long        output;
    ASIOError   err;

    input = 0;
    output = 0;
    err = ASIOGetChannels(&input, &output);
    return 0;
}

bool AsioDevice::start()
{
    ASIOError            err;
    ASIODriverInfo      *info;

    info = new ASIODriverInfo();
    err = ASIOInit(this->info);
    say("Asio version: " + QString::number(info->asioVersion));
    say("Asio Name: " + QString(info->name));
    this->info = info;
    (void) err;
    return true;
}

void AsioDevice::close()
{
    if (this->info)
    {
        ASIOStop();
        delete(this->info);
        this->info = NULL;
    }
    QIODevice::close();
}

void AsioDevice::say(const QString message)
{
    emit(debug("AsioDevice: " + message));
}

ModuleDevice* AsioDevice::factory(QString name, AudioFormat *format, void *userData, QObject *parent)
{
    AsioDevice *dev;

    (void) userData;
    (void) format;
    dev = new AsioDevice(parent);
    dev->setObjectName(name);
    return dev;
}

#endif
