#include "flowchecker.h"
#include "size.h"

#ifndef USER_H
#include "server/user.h"
#endif

FlowChecker::FlowChecker(AudioFormat *format, const int checkInterval, QObject *parent) :
    QObject(parent)
{
    this->format = format;
    timer.setParent(this);
    timer.setInterval(checkInterval);
    this->lastBytesRead = 0;
    connect(&timer,SIGNAL(timeout()),this,SLOT(check()));
}
FlowChecker::~FlowChecker() {
    stop();
}

bool FlowChecker::start() {
    if (!this->parent()) return false;
    else if (!format) return false;

    User* user = qobject_cast<User*>(this->parent());
    lastBytesRead = user->getBytesCount();
    warningCount = 0;
    timer.start();
    return true;
}
void FlowChecker::check() {
    //the parent will set this to 0 in case of user deletion
    if (!this->parent()) return;
    User* user = qobject_cast<User*>(this->parent());
    const int bytesRead = user->getBytesCount();

    const int neededSpeed = format->getBytesSizeForDuration(timer.interval());
    //here the +- tolerance is 20%
    const unsigned int maxSpeed = neededSpeed *1.2;
    const unsigned int minSpeed = neededSpeed *0.8;

    const unsigned int speed = bytesRead - lastBytesRead;

    //here we detect if the user is afk we kick him withous any warning
    if (!speed) {
        stop();
        emit(kick("afk"));
        return;
    }
    else if (!enableFlowKick); //DONT EVEN DARE TO PUT A RETURN HERE !!! (i'm serious ! we need fallback bellow)
    else if (speed > maxSpeed) {
        say("user is sending too much data: overflow attemp ? (" + Size::getWsize(speed) + " instead of " + Size::getWsize(neededSpeed) + ")");

        //in case of an overflow attemps, it's realy more dangerous than underflow so: banning the user for 2 mins
        if (warningCount++ > 3) {
            stop();
            emit(ban("overflow",120000));
            return;
        }
    }
    else if (speed < minSpeed) {
        say("user is not sending enoth data: buffer underflow prevention.");
        if (warningCount++ > 3) {
            stop();
            emit(kick("underflow"));
            return;
        }
    }
    else warningCount = 0;

    lastBytesRead = bytesRead;
}
void FlowChecker::say(const QString message) {
    emit (debug("FlowChecker: " + message));
}
int FlowChecker::getInterval() {
    return timer.interval();
}
void FlowChecker::setFlowKick(const bool mode) {
    this->enableFlowKick = mode;
}
void FlowChecker::stop() {
    timer.stop();
    timer.disconnect();
    lastBytesRead = 0;
    warningCount = 0;
}
