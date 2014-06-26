#ifdef PORTAUDIO

#include "portaudiodevice.h"
#include <portaudio.h>

#include <QDebug>

PortAudioDevice::PortAudioDevice(AudioFormat *format, QObject *parent) :
    QIODevice(parent)
{
}
PortAudioDevice::~PortAudioDevice() {
    Pa_Terminate();
    say("deleted");
}

bool PortAudioDevice::open(OpenMode mode) {
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        say("error: " + QString(Pa_GetErrorText(err)));
        return false;
    }
    QIODevice::open(mode);
    say("open ok");
    return true;
}
void PortAudioDevice::say(const QString message) {
    qDebug() << "PortAudioDevice: " + message;
}
qint64 PortAudioDevice::readData(char *data, qint64 maxlen) {
    return maxlen;
}
qint64 PortAudioDevice::writeData(const char *data, qint64 len) {
    return len;
}

#endif
