#include "moduledevice.h"

/*
 * This class is inherited by EVRY sound modules:
 * it's role is to port a "norm" in prevision to prevent the
 * "if forest" in the manager.cpp, and manager signal/slots
 */

ModuleDevice::ModuleDevice(QObject *parent) :
    QIODevice(parent)
{

}
int ModuleDevice::countDevices(QIODevice::OpenModeFlag mode)
{
    (void) mode;
    return 1;
}

bool ModuleDevice::setDeviceId(QIODevice::OpenModeFlag mode, const int id)
{
    (void) mode;
    (void) id;
    if (!id) return true;
    return false;
}

void ModuleDevice::say(QString message)
{
    (void) message;
}
