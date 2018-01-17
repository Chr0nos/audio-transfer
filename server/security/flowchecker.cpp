#include "flowchecker.h"
#include "size.h"
#include "server/user.h"

FlowChecker::FlowChecker(AudioFormat *format, const int checkInterval, QObject *parent) :
    QObject(parent)
{
    this->timer.setParent(this);
    this->timer.setInterval(checkInterval);
    this->setFormat(format);
    this->lastBytesRead = 0;
    connect(&timer,SIGNAL(timeout()),this,SLOT(check()));
    this->warningCount = 0;
    this->enableFlowKick = true;
}

FlowChecker::~FlowChecker()
{
    stop();
}

void FlowChecker::setFormat(AudioFormat *format)
{
    this->neededSpeed = format->getBytesSizeForDuration(this->timer.interval());
    //here the +- tolerance is 20%
    this->maxSpeed = this->neededSpeed *1.2;
    this->minSpeed = this->neededSpeed *0.8;
    this->format = format;
}

bool FlowChecker::start()
{
    User* user;

    if (!this->parent()) return false;
    else if (!format) return false;
    user = qobject_cast<User*>(this->parent());
    this->lastBytesRead = user->getBytesCount();
    this->warningCount = 0;
    this->timer.start();
    return true;
}

void FlowChecker::check()
{
    User* user;
    unsigned int speed;
    quint64 bytesRead;

    //the parent will set this to 0 in case of user deletion
    if (!this->parent()) return;
    else if (!this->format) return;
    user = qobject_cast<User*>(this->parent());
    bytesRead = user->getBytesCount();

    speed = bytesRead - this->lastBytesRead;

    //here we detect if the user is afk we kick him withous any warning
    if (!speed)
    {
        stop();
        emit(kick("afk"));
        return;
    }
    else if (!enableFlowKick); //DONT EVEN DARE TO PUT A RETURN HERE !!! (i'm serious ! we need fallback bellow)
    else if (speed > maxSpeed)
    {
        say("user is sending too much data: overflow attemp ? (" + Size::getWsize(speed) + " instead of " + Size::getWsize(this->neededSpeed) + ")");

        //in case of an overflow attemps, it's realy more dangerous than underflow so: banning the user for 2 mins
        if (warningCount++ > 3) {
            stop();
            emit(ban("overflow", 120000));
            return;
        }
    }
    else if (speed < minSpeed)
    {
        say("user is not sending enoth data: buffer underflow prevention.");
        if (warningCount++ > 3)
        {
            stop();
            emit(kick("underflow"));
            return;
        }
    }
    else warningCount = 0;
    lastBytesRead = bytesRead;
}

void FlowChecker::say(const QString message)
{
    emit (debug("FlowChecker: " + message));
}

int FlowChecker::getInterval()
{
    return timer.interval();
}

void FlowChecker::setFlowKick(const bool mode)
{
    this->enableFlowKick = mode;
}

void FlowChecker::stop()
{
    timer.stop();
    timer.disconnect();
    lastBytesRead = 0;
    warningCount = 0;
}
