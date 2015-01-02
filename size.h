#ifndef SIZE_H
#define SIZE_H

#include <QObject>
#include <QStringList>

class Size : public QObject
{
public:
    Size();
    static QString getWsize(const quint64 size,const int steps = 1024);
    static quint64 getRsize(const QString wsize);
    static QStringList getUnits();
};

#endif // SIZE_H
