#include "graphicgenerator.h"
#include "size.h"

#include <QPainter>
#include <QPixmap>

#include <QDebug>

GraphicGenerator::GraphicGenerator(QList<int> *speeds, QLabel *label, QObject *parent) :
    QObject(parent)
{
    this->label = label;
    this->speeds = speeds;
}
void GraphicGenerator::refresh() {
    /* cette fonction créé un graphique en batons pour afficher la vitesse du débit des données (enregisré dans le QList<int> "speeds"
     * 30% sont ajouté au maximal et 10 retiré au minimal pour améliorer la  lisibilitée
     * */

    if (speeds->count() < 2) return;
    //qDebug() << speeds;

    //on definis les valeurs de base qui ne changeront pas (taille du graphique, nombre de graduations, marges)
    const int hauteur = 400;
    const int largeur = 500;
    const int margeBas = 0;
    const int graduations = 20;

    //on détermine les valeurs mini et maxi de la liste speeds
    int maxValue = 0;
    int minValue = 0;

    //contiens la vitesse "moyene", j'ajoute chaque vitesse dans la boucle et je divise ensuite par le nombre d'items dans la liste "speeds"
    int average = 0;
    QList<int>::iterator v;
    for (v = speeds->begin() ; v != speeds->end() ; v++) {
        if (maxValue < *v) maxValue = *v;
        if (minValue > *v) minValue = *v;
        average += *v;
    }
    average /= speeds->count();
    minValue *= 0.9;
    if (minValue < 0) minValue = 0;
    maxValue *= 1.3;

    //ceci contiens l'écart entre la valeur maximale et la valeur minimale
    const int ecart = maxValue - minValue;

    //si l'écart est nul on arrete tout et on n'affiche rien: la liste est probablement vide ou ne contiens qu'une donnée (deux mini)
    if (!ecart) return;

    //on détermine le pas en pixels entre chaque graduations
    const int pasPx = hauteur / graduations;
    //ici le pas en unitées
    const int pasUnits = ecart / graduations;

    QPixmap pix(largeur,hauteur);

    QPainter p(&pix);
    QFont f;
    f.setFamily("Sans");
    f.setPixelSize(10);
    p.setFont(f);

    //definition d'un fond unifié (en blanc)
    QRect background;
    background.setCoords(0,0,largeur,hauteur);
    p.fillRect(background,Qt::white);

    //tracage des graduations et des unitées de mesures
    p.setPen(Qt::gray);
    for (int i = graduations ; i > 0 ; i--) {
        //ici la valeur qui sera affiché sur la graduation, je passe par un pointeur histoire de clarifier un peu ce que contiens i
        const int valeurGraduation = minValue + pasUnits * i;
        const int positionY = hauteur - (pasPx * i);

        //on trace la ligne de démarcation..... ETTTTT C'ESSTTTT LE BUUTTT !!!! ... non je décone, j'aime pas le foot :/
        p.drawLine(QPoint(10,positionY),QPoint(largeur,positionY));

        //definition de l'échelle sur la ligne (en ko/s normalement, dépendra du format audio choisi, peut etre en mb/s)
        p.drawText(QPoint(0,positionY),Size::getWsize(valeurGraduation) + "/s");
    }

    //ici on affiche les rectagles des valeurs de speeds
    p.setPen(Qt::gray);

    //on definis la position de la premiere colone à 50pixels sur la droite
    int pos = 62;
    for (v = speeds->begin() ; v != speeds->end() ; v++) {
        //const int relativeValue = maxValue - *v;
        const int valuePosition =  hauteur - hauteur * (*v - minValue) / ecart;
        //qDebug() << *v << relativeValue << valuePosition << unitPx;

        //création des coordonées du rectangle
        //pos contiens la position horizontale, valuePosition la position verticale, pos+5 pour une largeur de colone de 5pixels
        QRect r(QPoint(pos,valuePosition),QPoint(pos + 5,hauteur - margeBas));

        //on remplis le rectangle en noir
        p.fillRect(r,Qt::black);

        //on trace le contour du rectangle (en gris)
        //p.drawRect(r);

        //ici on modifie le déclage sur la droite (X) de 8 pixels
        pos += 8;
    }

    //on affiche la vitesse moyene (texte en haut à droite du graphique)
    p.setPen(Qt::blue);
    p.drawText(QPoint(largeur - 110,10),"Average: " + Size::getWsize(average) + "/s");

    //on trace une ligne bleu sur toute la largeur pour délimiter le niveau de la vitesse moyene
    const int yPos = hauteur - hauteur * (average - minValue) / ecart;
    p.drawLine(QPoint(0,yPos),QPoint(largeur,yPos));

    //on dessine tout ce bazard
    p.end();

    //on affiche le tout
    label->setPixmap(pix);
    label->setScaledContents(true);
}
