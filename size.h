#ifndef SIZE_H
#define SIZE_H

#include <QObject>
#include <QStringList>

class Size : public QObject
{
public:
    Size();
    static QString getWsize(const quint64 size);
    static quint64 getRsize(const QString wsize);
    static QStringList getUnits();
};

#endif // SIZE_H
