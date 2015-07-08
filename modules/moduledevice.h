#ifndef MODULEDEVICE_H
#define MODULEDEVICE_H

#include <QObject>
#include <QIODevice>
#include <QString>
#include "audioformat.h"

class ModuleDevice : public QIODevice
{
public:
    ModuleDevice(QObject *parent);
    virtual bool setDeviceId(QIODevice::OpenModeFlag mode, const int id);
    virtual int countDevices(QIODevice::OpenModeFlag mode);
    static ModuleDevice *factory(QString name, AudioFormat *format,void *userData, QObject *parent);
private:
    virtual void say(QString message);
signals:
    void debug(QString message);
    void error(int errorNo, QString message);
};

#endif // MODULEDEVICE_H
