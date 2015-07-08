#include "filedevice.h"
#include "manager.h"
#include <QFile>

FileDevice::FileDevice(QString filePath, QObject *parent) :
    ModuleDevice(parent)
{
    this->file = new QFile(filePath, parent);
}

FileDevice::~FileDevice()
{
    if (this->file)
    {
        this->file->close();
        this->file->disconnect();
        delete(this->file);
        this->file = NULL;
    }
}

bool FileDevice::open(OpenMode mode)
{
    if (this->file->open(mode))
    {
        ModuleDevice::open(mode);
        return true;
    }
    return false;
}

ModuleDevice* FileDevice::factory(QString name, AudioFormat *format, void *userData, QObject *parent)
{
    Manager::userConfig *config;
    FileDevice          *dev;

    config = (Manager::userConfig*) userData;
    dev = new FileDevice(*config->file.filePath, parent);
    (void) format;
    (void) name;
    return dev;
}

qint64 FileDevice::readData(char *data, qint64 maxlen)
{
    QByteArray read;
    qint64     lenght;
    if (!this->file->isReadable()) return -1;
    read = file->read(maxlen);
    lenght = read.length();
    memcpy(data, read.data(), lenght);
    return lenght;
}

qint64 FileDevice::writeData(const char *data, qint64 len)
{
    if (!this->file->isWritable()) return -1;
    return this->file->write(data, len);
}
void FileDevice::say(const QString message)
{
    (void) message;
    //emit(debug("FileDevice: " + message));
}
