#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QString>
#include "qrec.h"
#include "manager.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    QRec *rec;
    QString wsize(const quint64 size);

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
    void tcpTargetConnected();
    void refreshReadedData();

public slots:

private:
    Ui::MainWindow *ui;
    QTimer *timer;
    quint64 lastReadedValue;
    Manager *manager;
};


#endif // MAINWINDOW_H
