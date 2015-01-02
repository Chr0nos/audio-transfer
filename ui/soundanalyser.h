#ifndef SOUNDANALYSER_H
#define SOUNDANALYSER_H

#include <QDialog>

namespace Ui {
class SoundAnalyser;
}

class SoundAnalyser : public QDialog
{
    Q_OBJECT

public:
    explicit SoundAnalyser(QWidget *parent = 0);
    ~SoundAnalyser();
    void refresh();

private:
    Ui::SoundAnalyser *ui;
};

#endif // SOUNDANALYSER_H
