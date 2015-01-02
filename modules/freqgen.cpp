#include "freqgen.h"
#include "size.h"
#include <qmath.h>
#include <QDebug>

Freqgen::Freqgen(AudioFormat *format, QObject *parent) :
    QIODevice(parent)
{
    this->format = format;
    timer.setParent(this);
    timer.setInterval(50);
    connect(&timer,SIGNAL(timeout()),this,SIGNAL(readyRead()));
}
void Freqgen::close() {
    timer.stop();
    sample.clear();
    QIODevice::close();
}

bool Freqgen::open(OpenMode mode) {
    if (mode == QIODevice::ReadOnly) {
        QIODevice::open(mode);

        //demande d'un simple son à 440hz (un LA)
        //this->sample = getSample(440);
        //qDebug() << sample.toHex();
        //say("sample size: " + Size::getWsize(sample.size()) + " ,duration: 1sec");
        say("opened");
        lastReadTime = QTime::currentTime();
        readLeft = 0;
        currentPos = 0;
        timer.start();
        return true;
    }
    return false;
}
qint64 Freqgen::readData(char *data, qint64 maxlen) {
    //ici la taille requise pour tout avoir (elle peut etre superieure à maxlen, dans ce cas il faudra rappeler cette fonction juste apres)
    const int neededLen = bytesAvailable();

    //si aucun temps ne s'est écoulé: il n'y à rien à lire: on renvoi une erreur
    if (neededLen < 0) return -1;
    else if (!neededLen) return 0;

    //si les données requises ne rentrent pas dans le buffer aloué on lis en plusieures fois
    else if (maxlen > neededLen) {
        //say("split");
        const int missing = maxlen - neededLen;
        readLeft += missing;
        maxlen = neededLen;
    }
    else if (maxlen <= neededLen) {
        //say("reduce");
        readLeft -= maxlen;
        if (readLeft < 0) readLeft = 0;
    }

    float time;
    float amp = format->getSampleSize() * 1024;
    float freq = 440.f;
    float s;
    memset(data,0,maxlen);
    for (int i = 0 ; i < maxlen ; i++) {
        time = (float) currentPos / (float) format->getSampleRate();
        s = getValue(time,&freq,&amp);
        qint16 r = (qint16) qFloor(s);
        //on caste un char* en int*
        *(int*)&(data[i])=r;

        if (++currentPos > format->getSampleRate()) currentPos = 0;
    }
    //on definis le dernier temps de lecture
    lastReadTime = QTime::currentTime();

    //on retourne le nombre de données lues
    return maxlen;
}
qint64 Freqgen::writeData(const char *data, qint64 len) {
    //on ignore tout les parametres et on renvoi une erreur, cette fonction n'est la que pour que la classe compile sans erreurs
    //cette classe ne fonctione par definition qu'en mode ReadOnly
    (void) len;
    (void) data;
    return -1;
}
void Freqgen::say(const QString message) {
    emit(debug("Freqgen: " + message));
}
float Freqgen::getValue(float time, float *frequence, float *amplification) {
    const float pi = 3.14159265359;
    //DONT TOUCH: it's magic
    return qSin(time * *frequence * 2.f * pi) * *amplification;
}

QByteArray Freqgen::getSample(float frequence) {
    QByteArray sample;
    //facteur d'amplification
    float amp = format->getSampleSize() * 1024;

    say("amplification rate: " + QString::number(amp));

    //taux t'échantillonage par cycle
    float echantillonage = format->getSampleRate() / frequence;
    float s;

    //ici on génére une seconde.
    //step vaut un pas sur la fréquence (pour du 44100hz à 440hz step ira de 0 à 100)
    for (unsigned int step = 0 ; step < echantillonage ; step++) {
        s = getValue((float) step / format->getSampleRate(),
                     &frequence,
                     &amp);
        sample.append(QByteArray::number((qint16) s));
        //qDebug() << step << s;
    }
    return sample;
}
qint64 Freqgen::bytesAvailable() {
    //le temps écoulé depuis la derniere lecture en millisecondes
    const int elapsedTime =  lastReadTime.elapsed();
    if (!elapsedTime) return 0;
    return format->getBytesSizeForDuration(elapsedTime) + readLeft + QIODevice::bytesAvailable();
}
