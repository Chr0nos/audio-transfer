#include "soundanalyser.h"
#include "ui_soundanalyser.h"

SoundAnalyser::SoundAnalyser(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SoundAnalyser)
{
    ui->setupUi(this);
}

SoundAnalyser::~SoundAnalyser()
{
    delete ui;
}
SoundAnalyser::refresh() {
    QVector<double> x(101),y(101);
    for (int i = 0 ; i  < 101 ; i++) {
        x[i] = i/50.0 -1;
        y[i] = x[i] * x[i];
    }

}
