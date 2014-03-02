#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "qrec.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_refreshSources_clicked();
    void on_pushButton_clicked();
    void on_sourcesList_currentTextChanged();
    void on_browseSourceFilePath_clicked();
    void recStoped();
    bool isValidIp(const QString host);
    void on_sourcesList_currentIndexChanged(int index);
    void refreshEnabledSources();
    void refreshEnabledDestinations();

private:
    Ui::MainWindow *ui;
    QRec *rec;
};

#endif // MAINWINDOW_H
