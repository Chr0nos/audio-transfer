#ifndef PIPEDEVICE_H
#define PIPEDEVICE_H

#include <QIODevice>
#include <QString>
#include <QFile>
#include <QTimer>

class PipeDevice : public QIODevice
{
    Q_OBJECT
public:
    explicit PipeDevice(const QString name, QObject *parent = 0);
    ~PipeDevice();
    bool open(OpenMode mode);
    void say(const QString message);
private:
    QFile *file;
    QTimer *timer;
    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);

signals:
    void debug(const QString message);
public slots:
    void stop();

};

#endif // PIPEDEVICE_H
