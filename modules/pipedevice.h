#ifndef PIPEDEVICE_H
#define PIPEDEVICE_H

#include <QString>
#include <QFile>
#include <QThread>
#include "circularbuffer.h"
#include "modules/moduledevice.h"

class PipeDeviceRead : public QObject {
    Q_OBJECT
    public:
    explicit PipeDeviceRead(CircularBuffer* buffer,const int blocksize,QObject* parent);
    ~PipeDeviceRead();
private:
    CircularBuffer *buffer;
    int block;
    QFile *file;
public slots:
    void start();
private slots:
    void say(const QString message);
signals:
    void debug(const QString message);
    void failed();
};

class PipeDevice : public ModuleDevice
{
    Q_OBJECT
public:
    explicit PipeDevice(const QString name, QObject *parent = 0);
    ~PipeDevice();
    bool open(OpenMode mode);
    void setHexOutputEnabled(const bool mode);
    qint64 bytesAvailable();
    static ModuleDevice *factory(QString name, AudioFormat *format, void *userData, QObject *parent);
private:
    QFile *file;
    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);
    bool hexOutput;
    PipeDeviceRead* input;
    CircularBuffer *buffer;
    QThread *thread;

signals:
    void debug(const QString message);
public slots:
    void stop();
private slots:
    void say(const QString message);

};

#endif // PIPEDEVICE_H
