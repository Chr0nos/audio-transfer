#ifdef PHONON

#include "phonondevice.h"
#include <phonon/MediaObject>

PhononDevice::PhononDevice(QObject *parent) :
    QIODevice(parent)
{
}
bool PhononDevice::open(OpenMode mode) {
    QIODevice::open(mode);
    return false;
}
qint64 PhononDevice::readData(char *data, qint64 maxlen) {
    return maxlen;
}
qint64 PhononDevice::writeData(const char *data, qint64 len) {
    return len;
}

#endif
