#include "connector.h"
#include <QObject>

Connector::Connector()
{

}

void Connector::connect_buttons_sources(QList<QAbstractButton *> buttons, MainWindow *ui)
{
    QList<QAbstractButton*>::iterator i;

    i = buttons.begin();
    for (i = buttons.begin() ; i != buttons.end() ; i++)
    {
        QObject::connect((*i), SIGNAL(clicked(bool)), ui, SLOT(refreshEnabledSources()));
    }
}

void Connector::connect_buttons_destinations(QList<QAbstractButton *> buttons, MainWindow *ui)
{
    QList<QAbstractButton*>::iterator i;

    i = buttons.begin();
    for (i = buttons.begin() ; i != buttons.end() ; i++)
    {
        QObject::connect((*i), SIGNAL(clicked(bool)), ui, SLOT(refreshEnabledDestinations()));
    }
}
