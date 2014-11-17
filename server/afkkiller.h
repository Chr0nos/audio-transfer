#ifndef AFKKILLER_H
#define AFKKILLER_H

#include <QObject>
#include <QTimer>


class AfkKiller : public QObject
{
    Q_OBJECT
public:
    explicit AfkKiller(const int checkInterval,QObject *parent = 0);
    ~AfkKiller();
    void start();
    int getInterval();
private:
    quint64 lastBytesCount;
    QTimer* timer;
    int checkCount;
signals:
private slots:
    void checkAfk();
public slots:

};

#endif // AFKKILLER_H
