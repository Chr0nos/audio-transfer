#include "userhandler.h"
#include "size.h"
#include "server/servermain.h"
#include <QThread>

/*
** this class manage all connected users
** the user list is stored in this->users
** each user is a "User*" object
*/

UserHandler::UserHandler(QObject *parent) :
    QObject(parent)
{
    //bascicly: bytesRead store all the "offline" users data read (after they disconnected)
    this->bytesRead = 0;

}

User* UserHandler::createUser(QObject *socket, ServerSocket::type type, QString userName)
{
    User* user;

    threads = false;
    if (this->getIni()->getValue("general", "threads").toInt())
    {
        threads = true;
    }

    user = new User(socket, type, this);
    if (!user)
    {
        say("error: cannot create new user: failed to allocate memory");
        return 0;
    }
    connect(user, SIGNAL(sockClose(User*)), this, SLOT(sockClose(User*)));
    connect(user, SIGNAL(debug(QString)),this, SIGNAL(debug(QString)));
    connect(user, SIGNAL(kicked()), this, SLOT(kicked()));
    user->setObjectName(userName);
    this->users.append(user);
    if (this->threads)
    {
        this->moveUserToThread(user);
    }
    else user->start();
    return user;
}

void UserHandler::moveUserToThread(User *user)
{
    /*
    ** this method is called in threads mode:
    ** it move "user" to a new thread (created in this function)
    ** the clean of the QThread * is done by the signal "finished" received by the thread
    */
    QThread *thread;

    thread = new QThread(this);
    connect(thread, SIGNAL(started()), user, SLOT(start()));
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    user->setParent(0);
    user->moveToThread(thread);
    thread->start();
}

void UserHandler::say(const QString message) {
    emit(debug("UserHandler: " + message));
}

void UserHandler::sockClose(User *user) {
    const int pos = users.indexOf(user);

    if (pos < 0) return;
    //Adding the total bytes read of this user to the main counter (offline counter)
    bytesRead += user->getBytesCount();

    //deleteing the user from the user list
    users.removeAt(pos);

    say("deleting user: " + user->objectName());
    user->disconnect();
    delete(user);
    //re-assign the pointer value to NULL just in case
    user = NULL;
    //showing user actualy online and stats
    showUsersOnline();
}

void UserHandler::showUsersOnline()
{
    /*
     ** this function show all current connect users
     ** it will also show any "udp user" :
     ** an udp user is just an user who wasent kicked due to
     ** inactivity
     */
    int count;
    User *x;
    QList<User*>::Iterator i;

    say("current online users:");
    count = 0;
    for (i = users.begin() ; i != users.end() ; i++)
    {
        x = (User*) *i;
        say(QString::number(count++) + ": " + x->objectName() + QString(" -> readed: ") + Size::getWsize(x->getBytesCount()));
    }
    say("total readed data: " + Size::getWsize(getBytesRead()));
    if (!count) say("no user(s) online.");
    else say("end of list");
}

void UserHandler::killAll(const QString reason)
{
    /*
    ** this function kick evrybody from the server
    ** both udp and tcp users
    */
    QList<User*>::Iterator i;

    for (i = users.begin() ; i != users.end() ; i++)
    {
        (*i)->kill(reason);
    }
}

bool UserHandler::contains(const QObject *socket)
{
    /*
    ** this function return true if *socket
    ** belong to any user in the user list
    ** it's faster than indexOf
    */
    QList<User*>::Iterator i;

    for (i = users.begin() ; i != users.end() ; i++)
    {
        if ((*i)->getSocketPointer() == socket) return true;
    }
    return false;
}

int UserHandler::indexOf(const QObject *socket)
{
    /*
     ** this function return the position of an user
     ** in the user list using his socket pointer
     ** the socket could be a QTcpSocket* or a UdpSocket*
     ** if the user is not in the list, the function will
     ** return -1
     */
    int pos;
    QList<User*>::iterator i;

    i = users.begin();
    pos = -1;
    while (i != users.end())
    {
        pos++;
        if ((*i)->getSocketPointer() == socket) return pos;
        i++;
    }
    return -1;
}

int UserHandler::indexOf(const User *user)
{
    QList<User*>::iterator i;
    int pos;

    pos = -1;
    for (i = users.begin() ; i != users.end() ; i++)
    {
        pos++;
        if (*i == user) return pos;
    }
    return -1;
}

User* UserHandler::at(const int pos)
{
    /*
    ** this function return the user at "pos"
    ** the return is a User* described in server/user.cpp
    ** if the requested position is invalid, the function will
    ** return null
    */
    if (users.count() < pos) return NULL;
    return users.at(pos);
}

quint64 UserHandler::getBytesRead()
{
    return bytesRead + getBytesReadForConnected();
}

void UserHandler::kicked()
{
    /*
    ** this is a slot function
    ** it's role is to properly close the connection
    ** even if the client has not closed it properly
    ** or if he was kicked
    */
    User* user = (User*) sender();
    sockClose(user);
}

Readini* UserHandler::getIni() {
    return qobject_cast<ServerMain*>(this->parent())->getIni();
}

ServerSecurity* UserHandler::callSecurity() {
    return qobject_cast<ServerMain*>(this->parent())->security;
}

quint64 UserHandler::getBytesReadForConnected()
{
    /*
    ** this method return the total readed size
    ** so all user readed size combined
    */
    quint64 size;

    size = 0;
    QList<User*>::iterator i;
    for (i = users.begin() ; i != users.end() ; i++)
    {
        size += (*i)->getBytesCount();
    }
    return size;
}

int UserHandler::count()
{
    /*
    ** this method return the number of connected users
    ** it's also include udp users even if there is actualy
    ** no connection in udp
    */
    return users.count();
}

User* UserHandler::last()
{
    User *user;
    int count;

    user = 0;
    count = users.count();
    if (count)
    {
        user = users.at(count -1);
    }
    return user;
}
