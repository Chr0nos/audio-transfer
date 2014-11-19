#include "userhandler.h"
#include "size.h"

UserHandler::UserHandler(QObject *parent) :
    QObject(parent)
{
}
bool UserHandler::append(User *user) {

    this->users.append(user);
    this->bytesRead = 0;
    connect(user,SIGNAL(sockClose(User*)),this,SLOT(sockClose(User*)));
    connect(user,SIGNAL(debug(QString)),this,SIGNAL(debug(QString)));
    connect(user,SIGNAL(readedNewBytes(int)),this,SLOT(bytesNewRead(int)));
    connect(user,SIGNAL(kicked()),this,SLOT(kicked()));
    return true;
}
void UserHandler::say(const QString message) {
    emit(debug("UserHandler: " + message));
}
void UserHandler::sockClose(User *user) {
    const int pos = users.indexOf(user);
    if (pos < 0) return;
    say("deleting user: " + user->objectName());
    delete(users.at(pos));
    users.removeAt(pos);
    showUsersOnline();
}
void UserHandler::showUsersOnline() {
    say("current online users:");
    int count = 0;
    QList<User*>::Iterator i;
    for (i = users.begin() ; i != users.end() ; i++) {
        User* x = (User*) *i;
        say(QString::number(count++) + ": " + x->objectName() + QString(" -> readed: ") + Size::getWsize(x->getBytesCount()));
    }
    say("total readed data: " + Size::getWsize(bytesRead));
    if (!count) say("no user(s) online.");
    else say("end of list");
}
void UserHandler::killAll(const QString reason) {
    QList<User*>::Iterator i;
    for (i = users.begin() ; i != users.end() ; i++) {
        User* x = (User*) *i;
        x->kill(reason);
    }
}
bool UserHandler::contains(const QObject *socket) {
    QList<User*>::Iterator i;
    for (i = users.begin() ; i != users.end() ; i++) {
        User* x = (User*) *i;
        if (x->getSocketPointer() == socket) return true;
    }
    return false;
}
int UserHandler::indexOf(const QObject *socket) {
    int pos = -1;
    QList<User*>::Iterator i;
    for (i = users.begin() ; i != users.end() ; i++) {
        pos++;
        User* x = (User*) *i;
        if (x->getSocketPointer() == socket) return pos;
    }
    return -1;
}
User* UserHandler::at(const int pos) {
    if (users.count() < pos) return NULL;
    return users.at(pos);
}
quint64 UserHandler::getBytesRead() {
    return bytesRead;
}
void UserHandler::bytesNewRead(int size) {
    bytesRead += size;
}

void UserHandler::kicked() {
    User* user = (User*) sender();
    sockClose(user);
}
