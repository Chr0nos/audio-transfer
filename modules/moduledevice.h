#ifndef MODULEDEVICE_H
#define MODULEDEVICE_H

#include <QObject>
#include <QIODevice>
#include <QString>
#include "audioformat.h"

class ModuleDevice : public QIODevice
{
    Q_OBJECT
public:
    ModuleDevice(QObject *parent);
    virtual bool setDeviceId(QIODevice::OpenModeFlag mode, const int id);
    virtual int countDevices(QIODevice::OpenModeFlag mode);
    static ModuleDevice *factory(QString name, AudioFormat *format,void *userData, QObject *parent);
private:
    virtual void say(const QString message);
signals:
    void debug(const QString message);
    void error(int errorNo, QString message);
};

#endif // MODULEDEVICE_H
