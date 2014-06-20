#ifdef VLC

#include "vlcdevice.h"

VlcDevice::VlcDevice(QObject *parent) :
    QIODevice(parent)
{

}
bool VlcDevice::open(OpenMode mode) {

    return true;
}
void VlcDevice::close() {

}
qint64 VlcDevice::writeData(const char *data, qint64 len) {
    return len;
}
qint64 VlcDevice::readData(char *data, qint64 maxlen) {
    return maxlen;
}

#endif
