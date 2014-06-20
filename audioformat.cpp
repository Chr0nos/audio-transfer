#include "audioformat.h"

#include <QString>

AudioFormat::AudioFormat() {
    this->codec = "audio/pcm";
}
void AudioFormat::setChannelCount(const int channels) {
    this->channelsCount = channels;
}
void AudioFormat::setSampleRate(const int sampleRate) {
    this->sampleRate = sampleRate;
}
void AudioFormat::setSampleSize(const int sampleSize) {
    this->sampleSize = sampleSize;
}
int AudioFormat::getChannelsCount() {
    return channelsCount;
}
int AudioFormat::getSampleRate() {
    return sampleRate;
}
int AudioFormat::getSampleSize() {
    return sampleSize;
}
quint64 AudioFormat::getBitrate() {
    return sampleRate * sampleSize / 8 * channelsCount;
}
void AudioFormat::setCodec(const QString codec) {
    this->codec = codec;
}
bool AudioFormat::isValid() {
    return true;
}
QString AudioFormat::getCodec() {
    return codec;
}
quint64 AudioFormat::getBytesSizeForDuration(const int msecs) {
    //if duration == 0 i return the size of a trame
    if (!msecs) return sampleSize * channelsCount;
    return this->getBitrate() / 1000 * msecs;
}
const QString AudioFormat::getFormatTextInfo() {
    return QString("samplesize:" + QString::number(sampleSize) + " samplerate:" + QString::number(sampleRate) + " channels:" + QString::number(channelsCount));
}
bool AudioFormat::setFormat(AudioFormat *format) {
    sampleRate = format->getSampleRate();
    sampleSize = format->getSampleSize();
    codec = format->getCodec();
    channelsCount = format->getChannelsCount();
    return true;
}
