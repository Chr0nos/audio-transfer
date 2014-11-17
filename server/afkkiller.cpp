#include "afkkiller.h"
#include "server/user.h"
#include "size.h"

AfkKiller::AfkKiller(const int checkInterval, QObject *parent) :
    QObject(parent)
{
    this->checkCount = 0;
    timer = new QTimer(this);
    timer->setInterval(checkInterval);
    connect(timer,SIGNAL(timeout()),this,SLOT(checkAfk()));
}
AfkKiller::~AfkKiller() {
    this->timer->deleteLater();
}
void AfkKiller::start() {
    timer->start();
}

void AfkKiller::checkAfk() {
    User* user = qobject_cast<User*>(this->parent());
    const quint64 bytesRead = user->getBytesCount();


    //if no new data where readed: lets just kick the afk user
    if ((lastBytesCount == bytesRead) && (checkCount > 0)) {
        user->kill("afk");
    }

    //updating readed data
    lastBytesCount = bytesRead;

    //updating check counter
    checkCount++;
}
int AfkKiller::getInterval() {
    return timer->interval();
}