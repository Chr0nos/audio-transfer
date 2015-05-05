#ifndef VOLUMECHANGE_H
#define VOLUMECHANGE_H

#include <QIODevice>
#include <QByteArray>
#include "audioformat.h"

class Filters : public QObject
{
    Q_OBJECT
public:
    explicit Filters(QObject *parent = 0);
    static void volume(char** buffer,const size_t size,const float amplification,AudioFormat *format);
    static QByteArray intToCharArray(const int* data,size_t size,AudioFormat* format);

private:
signals:

public slots:

};

#endif // VOLUMECHANGE_H
