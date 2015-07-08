#ifndef FILEDEVICE_H
#define FILEDEVICE_H

#include <QObject>
#include <QFile>
#include "modules/moduledevice.h"

class FileDevice : public ModuleDevice
{
    //Q_OBJECT
public:
    FileDevice(QString filePath,AudioFormat *foramt, QObject *parent = 0);
    ~FileDevice();
    static ModuleDevice *factory(QString name, AudioFormat *format, void *userData, QObject *parent = 0);
    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);
    bool open(OpenMode mode);
private:
    QFile       *file;
    void say(const QString message);
    void writeWaveHeader();
    AudioFormat *format;
signals:
    void debug(const QString message);
};

#endif // FILEDEVICE_H
