#include "manager.h"
#include "devices.h"

//todo: this class will replace the whole Rec class (too bloated and nasty to evolut it)

Manager::Manager(QObject *parent) :
    QObject(parent)
{
    modeOutput = None;
    modeInput = None;
    devIn = 0;
    devOut = 0;
}
bool Manager::start() {
    if ((!devIn) || (!devOut)) return false;

    return true;
}
QStringList Manager::getDevicesNames(QAudio::Mode mode) {
    return Devices::getDevicesNames(mode);
}
