#ifndef AUDIOFORMAT_H
#define AUDIOFORMAT_H

#include <QObject>
#include <QString>

class AudioFormat
{

public:
    explicit AudioFormat();
    void setSampleRate(const int sampleRate);
    void setSampleSize(const int sampleSize);
    void setChannelCount(const int channels);
    int getSampleRate();
    int getSampleSize();
    int getChannelsCount();
    QString getCodec();
    quint64 getBitrate();
    void setCodec(const QString codec);
    bool isValid();
    quint64 getBytesSizeForDuration(const int msecs);
    const QString getFormatTextInfo();
    bool setFormat(AudioFormat *format);
private:
    int sampleRate;
    int sampleSize;
    int channelsCount;
    QString codec;
signals:

public slots:

};

#endif // AUDIOFORMAT_H
