#ifndef ASIODEVICE_H
#define ASIODEVICE_H

#include <QObject>
#include <QIODevice>
#include <QAbstractEventDispatcher>

#include <boost/asio/io_service.hpp>

class AsioDevice : public QIODevice
{
private:
    boost::asio::io_service io_service;
public:
    AsioDevice();
    bool open(OpenMode mode);
    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);
};

#endif // ASIODEVICE_H
