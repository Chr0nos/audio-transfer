#ifndef READINI_H
#define READINI_H

#include <QObject>
#include <QMap>
#include <QString>
#include <QStringList>

class Readini : public QObject
{
    Q_OBJECT
public:
    explicit Readini(const QString filePath,QObject *parent = 0);
    void parseIni();
    QStringList getSections();
    QStringList getKeys(const QString section);
    QString getValue(const QString section,const QString key);
    bool isSection(const QString section);
    bool isKey(const QString section,const QString key);
    void setValue(const QString section,const QString item,const QString value);
    void setValue(const QString section, const QString item, const int value);
    bool removeItem(const QString section,const QString item);
    bool removeSection(const QString section);
    bool flush();
    bool exists();

private:
    QString filePath;
    QMap<QString,QMap<QString,QString> > iniContent;

signals:

public slots:

};

#endif // READINI_H
