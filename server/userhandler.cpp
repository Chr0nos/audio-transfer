#include "userhandler.h"

UserHandler::UserHandler(QObject *parent) :
    QObject(parent)
{
}
bool UserHandler::append(User *user) {

    this->users.append(user);
    connect(user,SIGNAL(sockClose(User*)),this,SLOT(sockClose(User*)));
    connect(user,SIGNAL(debug(QString)),this,SIGNAL(debug(QString)));
    return true;
}
void UserHandler::say(const QString message) {
    emit(debug("UserHandler: " + message));
}
void UserHandler::sockClose(User *user) {
    if (!users.contains(user)) return;
    say("deleting user: " + user->getUserName());
    const int pos = users.indexOf(user);
    users.at(pos)->deleteLater();
    users.removeAt(pos);
}
void UserHandler::showUsersOnline() {
    say("current online users:");
    int count = 0;
    QList<User*>::Iterator i;
    for (i = users.begin() ; i != users.end() ; i++) {
        User* x = (User*) *i;
        say(QString::number(count++) + ": " + x->objectName());
    }
    say("end of list");
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
    quint64 bytesRead = 0;
    QList<User*>::Iterator i;
    for (i = users.begin() ; i != users.end() ; i++) {
        User* x = (User*) *i;
        bytesRead += x->getBytesCount();
    }
    return bytesRead;
}
