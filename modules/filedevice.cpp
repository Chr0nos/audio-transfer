#include "filedevice.h"
#include "manager.h"
#include <QFile>

FileDevice::FileDevice(QString filePath, AudioFormat *format, QObject *parent) :
    ModuleDevice(parent)
{
    this->file = new QFile(filePath, parent);
    this->format = format;
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
    dev = new FileDevice(*config->file.filePath,format, parent);
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
    quint64 wrote;

    if (!this->file->isWritable()) return -1;
    wrote = this->file->write(data, len);
    //this->writeWaveHeader();
    return wrote;
}
void FileDevice::say(const QString message)
{
    (void) message;
    //emit(debug("FileDevice: " + message));
}

void FileDevice::writeWaveHeader()
{
    /*
    ** this method write the current header in the file
    ** i used the tutorial at url:
    ** http://mathmatrix.narod.ru/Wavefmt.html
    */
    qint64      pos;
    QByteArray  header;

    pos = this->file->pos();
    header.append("RIFF");                                                       //file description header
    header.append(pos - 8);                                                      //file size not including header
    header.append("WAVE");                                                       //WAV description header
    header.append("fmt ");                                                       //format description header
    header.append(16);                                                           //size of WAVE section chunck
    header.append(010);                                                          //PCM
    header.append(this->format->getChannelsCount());                             //channels
    header.append(this->format->getSampleRate());                                //samples per second
    header.append(this->format->getBitrate());                                   //bytes per seconds
    header.append(format->getChannelsCount() * format->getSampleSize() / 8);     //block aligmement
    header.append(format->getSampleSize());                                      //bits per sample
    header.append("data");                                                       //data description header
    header.append(pos - header.size() - 4);                                      //size of data

    this->file->seek(0);
    this->file->write(header);
    this->file->seek(pos);
}
