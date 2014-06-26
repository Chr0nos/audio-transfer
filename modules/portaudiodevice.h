#ifdef PORTAUDIO

#ifndef PORTAUDIODEVICE_H
#define PORTAUDIODEVICE_H

#include <QIODevice>
#include <QString>

#include "audioformat.h"

class PortAudioDevice : public QIODevice
{
    Q_OBJECT
public:
    explicit PortAudioDevice(AudioFormat *format,QObject *parent = 0);
    ~PortAudioDevice();
    bool open(OpenMode mode);
    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);
private:
    void say(const QString message);
signals:

public slots:

};

#endif // PORTAUDIODEVICE_H

#endif
