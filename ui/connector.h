#ifndef CONNECTOR_H
#define CONNECTOR_H

#include <QMainWindow>
#include <QObject>
#include <QList>
#include <QAbstractButton>
#include "mainwindow.h"

class Connector
{
public:
    Connector();
    static void connect_buttons_sources(QList<QAbstractButton*> buttons, MainWindow* ui);
    static void connect_buttons_destinations(QList<QAbstractButton*> buttons, MainWindow* ui);

};

#endif // CONNECTOR_H
