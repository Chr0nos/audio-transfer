#include "flowchecker.h"

#ifndef USER_H
#include "server/user.h"
#endif

#include <QDebug>

FlowChecker::FlowChecker(AudioFormat *format, const int checkInterval, QObject *parent) :
    QObject(parent)
{
    this->format = format;
    timer.setParent(this);
    timer.setInterval(checkInterval);
    connect(&timer,SIGNAL(timeout()),this,SLOT(check()));
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
    User* user = qobject_cast<User*>(this->parent());
    const int bytesRead = user->getBytesCount();

    const int neededSpeed = format->getBytesSizeForDuration(timer.interval());
    //here the +- tolerance is 20%
    const int maxSpeed = neededSpeed *1.2;
    const int minSpeed = neededSpeed *0.8;

    const int speed = bytesRead - lastBytesRead;

    if (!speed) user->kill("afk");
    else if (!enableFlowKick);
    else if (speed > maxSpeed) {
        say("user is sending too much data: overflow attemp ?");

        //in case of an overflow attemps, it's realy more dangerous than underflow so: banning the user for 2 mins
        if (warningCount++ > 3) user->ban("overflow",120000);
    }
    else if (speed < minSpeed) {
        say("user is not sending enoth data: buffer underflow prevention.");
        if (warningCount++ > 3) user->kill("underflow");
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
