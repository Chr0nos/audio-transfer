#ifndef MANAGER_H
#define MANAGER_H

#include "devices.h"

#include <QObject>
#include <QIODevice>

class Manager : public QObject
{
    Q_OBJECT
public:
    enum Mode {File = 0,Device = 1,Tcp = 2,None = 3};
    struct userConfig {
       Mode modeInput;
       Mode modeOutput;
       QString codec;
       int sampleRate;
       int sampleSize;
    };

    explicit Manager(QObject *parent = 0);
    bool start();
    void stop();
    static QStringList getDevicesNames(QAudio::Mode mode);

private:
    Mode modeInput;
    Mode modeOutput;
    bool openInput();
    bool openOutput();
    QIODevice *devIn;
    QIODevice *devOut;


signals:

public slots:

};

#endif // MANAGER_H
