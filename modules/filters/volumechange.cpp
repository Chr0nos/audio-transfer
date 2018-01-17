#include "volumechange.h"
#include <QByteArray>

Filters::Filters(QObject *parent) :
    QObject(parent)
{
}

void Filters::volume(char **buffer, const size_t size, const float amplification, AudioFormat *format) { 
    const int	max = format->getSampleSize() * 1024;
    const int	min = max * -1;
    qint16 		c;

    for (size_t i = 0 ; i < size ; i++)
	{
        c = *buffer[i] << 8;
        c = c|*buffer[i+1];
        c *= amplification;

        //ici on définis un écrétage pour éviter de dépasser les valeurs mini et maxi authorisées par le samplesize
        if (c < min)
		{
			c = min;
		}
        else if (c > max)
		{
			c = max;
		}
    }
}

QByteArray Filters::intToCharArray(const int *data, size_t size, AudioFormat *format) {
    int			bytes;
    size_t 		bufferSize;
    char		*buffer;
    int			bufferPos;
    char 		x;
 	QByteArray	result;

	bytes = format->getSampleSize() / 4;
	bufferSize = size * bytes;
	buffer = new char(bufferSize);
	bufferPos = 0;
	x = 0;
	//cette boucle parcourt le buffer d'entrée (le int*)
    for (unsigned int i = 0 ; i < size ; i += bytes)
	{
        //ici cette boucle s'occupe de découper les intt(32) (4 octects) en blocs de 2 octects pour tenir dans des char
        //pour un sample size de  16 bits aura donc bits = 3 2 1 0
        //pour 32bits on aura: 7 6 5 4 3 2 1 0
        for (int bit = bytes -1 ; bit > 0 ; bit--)
		{
			x = (data[i + bit] << (bit * 8));
        }
        buffer[bufferPos++] = x;
    }
    result = QByteArray(buffer,bufferSize);
	delete(buffer);
	return result;
}

