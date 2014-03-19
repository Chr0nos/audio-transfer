#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "qrec.h"
#include "manager.h"

#include <QString>
#include <QtMultimedia/QAudioOutput>
#include <QFileDialog>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    rec = new QRec(this);
    manager = new Manager(this);

    timer = new QTimer(this);
    timer->setInterval(1000);
    connect(rec,SIGNAL(stoped()),this,SLOT(recStoped()));
    on_refreshSources_clicked();
    ui->destinationDeviceCombo->addItems(Manager::getDevicesNames(QAudio::AudioOutput));

    connect(ui->sourceRadioDevice,SIGNAL(clicked()),this,SLOT(refreshEnabledSources()));
    connect(ui->sourceRadioFile,SIGNAL(clicked()),this,SLOT(refreshEnabledSources()));
    connect(ui->sourceRadioTcp,SIGNAL(clicked()),this,SLOT(refreshEnabledSources()));
    connect(ui->destinationDeviceRadio,SIGNAL(clicked()),this,SLOT(refreshEnabledDestinations()));
    connect(ui->destinationRadioFile,SIGNAL(clicked()),this,SLOT(refreshEnabledDestinations()));
    connect(ui->destinationRadioTcp,SIGNAL(clicked()),this,SLOT(refreshEnabledDestinations()));
    connect(rec,SIGNAL(targetConnected()),this,SLOT(tcpTargetConnected()));
    connect(timer,SIGNAL(timeout()),this,SLOT(refreshReadedData()));

}

MainWindow::~MainWindow()
{
    delete rec;
    delete manager;
    delete ui;
}

void MainWindow::on_refreshSources_clicked()
{
    ui->sourcesList->clear();
    ui->sourcesList->addItems(Manager::getDevicesNames(QAudio::AudioInput));
}


void MainWindow::on_pushButton_clicked()
{
    //record button.
    if (!rec->isRecording()) {
        QRec::userConfig config;
        config.channels = ui->channelsCount->value();
        config.sampleRate = ui->samplesRates->currentText().toInt();
        config.sampleSize = ui->samplesSize->currentText().toInt();
        config.codec = ui->codecList->currentText();
        rec->setUserConfig(config);

        if (ui->sourceRadioFile->isChecked()) {
            qDebug() << "ui: file source mode";
            rec->setSourceFilePath(ui->sourceFilePath->text());
        }
        else if (ui->sourceRadioDevice->isChecked()) {
            if (rec->setSourceId(ui->sourcesList->currentIndex())) ui->statusBar->showMessage("Source device opened",3000);
            else {
                ui->statusBar->showMessage("error: cannot open source device",3000);
                return;
            }
        }
        else if (ui->sourceRadioTcp->isChecked()) {
            rec->setSourceTcp(ui->sourceTcpHostAllowed->text(),ui->sourceTcpHostPort->value());
            rec->setTcpOutputBuffer(ui->destinationTcpBufferDuration->value());
        }

        if (ui->destinationRadioFile->isChecked()) {
            rec->setTargetFilePath(ui->destinationFilePath->text());
            rec->startRecAlt();
        }
        else if (ui->destinationDeviceRadio->isChecked()) {
            //this feature is EXPERIMENTAL, may memory leak, crash, and other dirty stuff: please future me: forgive me...
            QAudioFormat format;
            format.setChannelCount(ui->channelsCount->value());
            format.setSampleRate(ui->samplesRates->currentText().toInt());
            format.setCodec(ui->codecList->currentText());
            QAudioDeviceInfo info = rec->getAudioDeviceById(ui->destinationDeviceCombo->currentIndex(),QAudio::AudioOutput);
            if (!info.isFormatSupported(format)) {
                format = info.nearestFormat(format);
            }
            QAudioOutput *audioOut = new QAudioOutput(info,format,this);
            QIODevice *devOut = audioOut->start();

            rec->setAudioOutput(devOut);
            ui->statusBar->showMessage("redirecting from " + ui->sourcesList->currentText() + " to " + ui->destinationDeviceCombo->currentText());
        }
        else if (ui->destinationRadioTcp->isChecked()) {
            const QString host = ui->destinationTcpSocket->text().split(":").first();
            const int port = ui->destinationTcpSocket->text().split(":").last().toInt();
            if (!isValidIp(host)) {
                ui->statusBar->showMessage("Error: invalid target ip: refusing to connect");
                return;
            }
            if ((port >= 65535) || (port <= 0)) {
                ui->statusBar->showMessage("Error: invalid port: refusing to connect");
                return;
            }
            ui->statusBar->showMessage("Connecting to " + host + " on port " + QString().number(port));
            rec->setTargetTcp(host,port);

        }

        if (rec->startRecAlt()) {
            ui->pushButton->setText("Stop");
            lastReadedValue = 0;
            timer->start();
        }
        else ui->statusBar->showMessage("Failed to open the source device.",3000);
    }
    else {
        rec->stopRec();
        ui->pushButton->setText("Record");
    }
}

void MainWindow::on_sourcesList_currentTextChanged()
{
    ui->codecList->clear();
    ui->samplesRates->clear();
    ui->samplesSize->clear();
    if (ui->sourcesList->currentIndex() >= 0) {
        ui->pushButton->setEnabled(true);
        ui->codecList->addItems(rec->getSupportedCodec());
        ui->codecList->setCurrentIndex(0);
        ui->samplesSize->addItems(rec->getSupportedSamplesSizes());
        ui->samplesRates->addItems(rec->getSupportedSamplesRates());

        const int goodRatePos = ui->samplesRates->findText("44100");
        if (goodRatePos) ui->samplesRates->setCurrentIndex(goodRatePos);
        else ui->samplesRates->setCurrentIndex(ui->samplesRates->count() -1);
    }
    else ui->pushButton->setEnabled(false);
}

void MainWindow::on_browseSourceFilePath_clicked()
{
    QFileDialog sel(this);
    sel.setFilter(QDir::Files);
    sel.setModal(true);
    sel.show();
    sel.exec();
    if (sel.result() == QFileDialog::Rejected) return;
    else if (!sel.selectedFiles().isEmpty()) ui->sourceFilePath->setText(sel.selectedFiles().first());
}


void MainWindow::recStoped() {
    timer->stop();
    ui->statusBar->showMessage("Record stoped",3000);
    ui->pushButton->setText("Record");
}

bool MainWindow::isValidIp(const QString host) {
    //return true if the host has a valid ip, else false, valid ip example: 192.168.1.1
    const QStringList x = host.split(".");
    if (x.count() != 4) return false;
    int v;
    foreach (QString f,x) {
        v = f.toInt();
        if (v > 255) return false;
        else if (v < 0) return false;
    }
    return true;
}

void MainWindow::on_sourcesList_currentIndexChanged(int index)
{
    ui->samplesRates->clear();
    ui->codecList->clear();
    if (index >= 0) {
        rec->setSourceId(index);
        ui->samplesRates->addItems(rec->getSupportedSamplesRates());
        ui->codecList->addItems(rec->getSupportedCodec());
        //ui->channelsCount->setMaximum(rec->getMaxChannelsCount());
    }
}
void MainWindow::refreshEnabledSources() {
    ui->refreshSources->setEnabled(false);
    ui->sourceFilePath->setEnabled(false);
    ui->sourcesList->setEnabled(false);
    ui->browseSourceFilePath->setEnabled(false);
    ui->sourceTcpHostAllowed->setEnabled(false);
    ui->sourceTcpHostPort->setEnabled(false);
    if (ui->sourceRadioDevice->isChecked()) {
        ui->sourcesList->setEnabled(true);
        ui->refreshSources->setEnabled(true);
    }
    else if (ui->sourceRadioFile->isChecked()) {
        ui->sourceFilePath->setEnabled(true);
        ui->browseSourceFilePath->setEnabled(true);
    }
    else if (ui->sourceRadioTcp->isChecked()) {
        ui->sourceTcpHostAllowed->setEnabled(true);
        ui->sourceTcpHostPort->setEnabled(true);
    }
}
void MainWindow::refreshEnabledDestinations() {
    ui->destinationFilePath->setEnabled(false);
    ui->destinationPathBrowse->setEnabled(false);
    ui->destinationTcpBufferDuration->setEnabled(false);
    ui->destinationTcpSocket->setEnabled(false);
    ui->destinationDeviceCombo->setEnabled(false);

    if (ui->destinationDeviceRadio->isChecked()) {
        ui->destinationDeviceCombo->setEnabled(true);
    }
    else if (ui->destinationRadioFile->isChecked()) {
        ui->destinationFilePath->setEnabled(true);
        ui->destinationPathBrowse->setEnabled(true);
    }
    else if (ui->destinationRadioTcp->isChecked()) {
        ui->destinationTcpSocket->setEnabled(true);
        ui->destinationTcpBufferDuration->setEnabled(true);
    }
}
void MainWindow::tcpTargetConnected() {
    ui->statusBar->showMessage("Target connected",3000);
}
void MainWindow::refreshReadedData() {
    quint64 readed = rec->getReadedData();
    int speed = readed - lastReadedValue;
    ui->statusBar->showMessage("Readed data: " + wsize(rec->getReadedData()) + " - speed: " + wsize(speed) + "/s");

    lastReadedValue = rec->getReadedData();
}
QString MainWindow::wsize(const quint64 size) {
    double isize = size;
    QStringList keys;
    keys << "o" << "Kb" << "Mb" << "Gb" << "Tb" << "Pb" << "Eb" << "Zb" << "Yb";
    int n;
    for (n = 0;isize >= 1024;n++) isize = isize / 1024;
    if (n >= keys.count()) n = keys.count() -1;
    return QString::number(isize,10,2) + keys.at(n);
}
