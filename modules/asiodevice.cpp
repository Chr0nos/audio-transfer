#include "asiodevice.h"
#include <boost/asio.hpp>

AsioDevice::AsioDevice()
{

}

qint64 AsioDevice::readData(char *data, qint64 maxlen)
{
    (void) data;
    (void) maxlen;
    return 0;
}

qint64 AsioDevice::writeData(const char *data, qint64 len)
{
    (void) data;
    (void) len;
    return 0;
}

bool AsioDevice::open(OpenMode mode)
{
    (void) mode;
    return true;
}
