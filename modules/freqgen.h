#ifndef FREQGEN_H
#define FREQGEN_H

#include <QByteArray>
#include <QTime>
#include <QTimer>
#include "audioformat.h"
#include "modules/moduledevice.h"

class Freqgen : public ModuleDevice
{
    Q_OBJECT
public:
    explicit Freqgen(AudioFormat *format,QObject *parent = 0);
    bool open(OpenMode mode);
    void close();
    qint64 bytesAvailable();
    QByteArray generateSound(int duration, float *frequence);
    QByteArray generateSample(float *frequence);
    void setSample(float frequence);
    static ModuleDevice *factory(QString name, AudioFormat *format, void *userData, QObject *parent);
private:
    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);
    void say(const QString message);
    QByteArray getSample(float frequence);
    QByteArray sample;
    AudioFormat *format;
    QTime lastReadTime;
    QTimer timer;
    int readLeft;
    float getValue(unsigned int *time, float* frequence, float* amplification);
    float getValue(float *time,float* frequence,float* amplification);
    int currentPos;
    void duplicateSoundForChannels(const short *channels, QByteArray *target, int *value, unsigned int *position);
    float pi;

signals:
    void debug(const QString message);
public slots:

};

#endif // FREQGEN_H
