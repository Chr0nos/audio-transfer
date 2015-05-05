#include "volumechange.h"

Filters::Filters(QObject *parent) :
    QObject(parent)
{
}
void Filters::volume(char **buffer, const size_t size, const float amplification, AudioFormat *format) {
    //const int bytes = format->getSampleRate() / 4;
    const int max = format->getSampleSize() * 1024;
    const int min = max * -1;
    qint16 c;
    for (size_t i = 0 ; i < size ; i++) {
        c = *buffer[i] << 8;
        c = c|*buffer[i+1];
        c *= amplification;

        //ici on définis un écrétage pour éviter de dépasser les valeurs mini et maxi authorisées par le samplesize
        if (c < min) c = min;
        else if (c > max) c = max;
    }
}

QByteArray Filters::intToCharArray(const int *data, size_t size, AudioFormat *format) {
    //ici on calcule sur combiens d'octects on va devoir travailler
    const int bytes = format->getSampleSize() / 4;

    const size_t bufferSize = size * bytes;
    char buffer[bufferSize];
    int bufferPos = 0;
    char x = 0;
    //cette boucle parcourt le buffer d'entrée (le int*)
    for (unsigned int i = 0 ; i < size ; i += bytes) {

        //ici cette boucle s'occupe de découper les int(32) (4 octects) en blocs de 2 octects pour tenir dans des char*
        //pour un sample size de  16 bits aura donc bits = 3 2 1 0
        //pour 32bits on aura: 7 6 5 4 3 2 1 0
        for (int bit = bytes -1 ; bit > 0 ; bit--) {

            //x est un char , data un int*
            x = (data[i + bit] << (bit * 8));
        }

        //on ajoute alors x au buffer en incrémentant la position actuele dans le dit buffer
        buffer[bufferPos++] = x;
    }
    //on créé un QByteArray depuis le buffer du char*
    return QByteArray(buffer,bufferSize);
}
