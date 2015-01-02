#ifndef FREQGEN_H
#define FREQGEN_H

#include <QIODevice>
#include <QByteArray>
#include <QTime>
#include <QTimer>
#include "audioformat.h"

class Freqgen : public QIODevice
{
    Q_OBJECT
public:
    explicit Freqgen(AudioFormat *format,QObject *parent = 0);
    bool open(OpenMode mode);
    void close();
    qint64 bytesAvailable();
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
    float getValue(float time,float* frequence,float* amplification);
    int currentPos;


signals:
    void debug(const QString message);
public slots:

};

#endif // FREQGEN_H
