#ifndef GRAPHICGENERATOR_H
#define GRAPHICGENERATOR_H

#include <QObject>
#include <QLabel>

class GraphicGenerator : public QObject
{
    Q_OBJECT
public:
    explicit GraphicGenerator(QList<int> *speeds,QLabel* label,QObject *parent = 0);
    void refresh();
private:
    QLabel *label;
    QList<int> *speeds;
signals:

public slots:

};

#endif // GRAPHICGENERATOR_H
