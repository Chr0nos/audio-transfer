#include "mainwindow.h"
#include "ui_mainwindow.h"
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

    manager = new Manager(this);

    timer = new QTimer(this);
    timer->setInterval(1000);
    connect(manager,SIGNAL(stoped()),this,SLOT(recStoped()));
    on_refreshSources_clicked();
    ui->destinationDeviceCombo->addItems(Manager::getDevicesNames(QAudio::AudioOutput));

    connect(ui->sourceRadioDevice,SIGNAL(clicked()),this,SLOT(refreshEnabledSources()));
    connect(ui->sourceRadioFile,SIGNAL(clicked()),this,SLOT(refreshEnabledSources()));
    connect(ui->destinationDeviceRadio,SIGNAL(clicked()),this,SLOT(refreshEnabledDestinations()));
    connect(ui->destinationRadioFile,SIGNAL(clicked()),this,SLOT(refreshEnabledDestinations()));
    connect(ui->destinationRadioTcp,SIGNAL(clicked()),this,SLOT(refreshEnabledDestinations()));
    connect(ui->destinationRadioPulseAudio,SIGNAL(clicked()),SLOT(refreshEnabledDestinations()));
    connect(manager,SIGNAL(tcpTargetConnected()),this,SLOT(tcpTargetConnected()));
    connect(manager,SIGNAL(errors(QString)),this,SLOT(errors(QString)));
    connect(manager,SIGNAL(started()),this,SLOT(started()));
    connect(manager,SIGNAL(debug(QString)),this,SLOT(debug(QString)));
    connect(ui->samplesRates,SIGNAL(currentIndexChanged(int)),this,SLOT(refreshEstimatedBitrate()));
    connect(ui->samplesSize,SIGNAL(currentIndexChanged(int)),this,SLOT(refreshEstimatedBitrate()));
    connect(ui->channelsCount,SIGNAL(valueChanged(int)),this,SLOT(refreshEstimatedBitrate()));
    connect(timer,SIGNAL(timeout()),this,SLOT(refreshReadedData()));
#ifndef PULSE
    ui->destinationRadioPulseAudio->setEnabled(false);
    ui->destinationRadioPulseAudio->deleteLater();
    ui->destinationPulseAudioLineEdit->deleteLater();
#endif

}

MainWindow::~MainWindow()
{
    delete manager;
    delete ui;
}
void MainWindow::errors(const QString error) {
    ui->statusBar->showMessage(error,3000);
    this->debug(error);
}
void MainWindow::debug(const QString message) {
    ui->debug->addItem(message);
}

void MainWindow::on_refreshSources_clicked()
{
    ui->sourcesList->clear();
    ui->sourcesList->addItems(Manager::getDevicesNames(QAudio::AudioInput));
}


void MainWindow::on_pushButton_clicked()
{
    //record button.
    if (!manager->isRecording()) {
        ui->debug->clear();
        const int bitrate = (ui->samplesRates->currentText().toInt() * ui->samplesSize->currentText().toInt() / 8) * ui->channelsCount->value();
        debug("estimated bitrate: " + wsize(bitrate));
        Manager::userConfig mc;
        mc.codec = ui->codecList->currentText();
        mc.modeInput = Manager::None;
        mc.modeOutput = Manager::None;
        mc.sampleRate = ui->samplesRates->currentText().toInt();
        mc.sampleSize = ui->samplesSize->currentText().toInt();
        mc.channels = ui->channelsCount->value();
        mc.filePathOutput = ui->sourceFilePath->text();
        mc.devices.input = ui->sourcesList->currentIndex();
        mc.devices.output = ui->destinationDeviceCombo->currentIndex();
        mc.bufferSize = bitrate * ui->destinationTcpBufferDuration->value() / 1000 / 100;
        mc.bufferMaxSize = 2097152; //2Mb
        debug("buffer size: " + wsize(mc.bufferSize));

        if (ui->sourceRadioFile->isChecked()) {
            qDebug() << "ui: file source mode";
            mc.modeInput = Manager::File;
        }
        else if (ui->sourceRadioDevice->isChecked()) mc.modeInput = Manager::Device;


        if (ui->destinationRadioFile->isChecked()) mc.modeInput = Manager::File;
        else if (ui->destinationDeviceRadio->isChecked()) {
            mc.modeOutput = Manager::Device;
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
            mc.tcpTarget.host = host;
            mc.tcpTarget.port = port;
            mc.tcpTarget.sendConfig = true;
            mc.modeOutput = Manager::Tcp;

            ui->statusBar->showMessage("Connecting to " + host + " on port " + QString().number(port));
        }
        else if (ui->destinationRadioPulseAudio->isChecked()) {
            debug("using pulseaudio target: this is an experimental feature!");
            if (!ui->destinationPulseAudioLineEdit->text().isEmpty()) {
                mc.pulseTarget = ui->destinationPulseAudioLineEdit->text();
            }
            mc.modeOutput = Manager::PulseAudio;

        }
        manager->setUserConfig(mc);
        manager->start();
    }
    else {
        manager->stop();
        ui->pushButton->setText("Record");
    }
}

void MainWindow::on_sourcesList_currentTextChanged()
{
    ui->codecList->clear();
    ui->samplesRates->clear();
    ui->samplesSize->clear();
    if (ui->sourcesList->currentIndex() >= 0) {
        QAudioDeviceInfo info = QAudioDeviceInfo::availableDevices(QAudio::AudioInput).at(ui->sourcesList->currentIndex());
        ui->pushButton->setEnabled(true);
        ui->codecList->addItems(info.supportedCodecs());
        ui->codecList->setCurrentIndex(0);
        ui->samplesSize->addItems(Manager::intListToQStringList(info.supportedSampleSizes()));
        ui->samplesRates->addItems(Manager::intListToQStringList(info.supportedSampleRates()));
        ui->channelsCount->setMaximum(info.supportedChannelCounts().last());

        //setting current config as the best possible
        ui->samplesSize->setCurrentIndex(ui->samplesSize->count() -1);
        ui->samplesRates->setCurrentIndex(ui->samplesRates->count() -1);
        ui->channelsCount->setValue(info.supportedChannelCounts().last());
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
    setUserControlState(true);
    refreshEnabledSources();
    refreshEnabledDestinations();
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
        QAudioDeviceInfo info = QAudioDeviceInfo::availableDevices(QAudio::AudioInput).at(ui->sourcesList->currentIndex());
        ui->samplesRates->addItems(Manager::intListToQStringList(info.supportedSampleRates()));
        ui->codecList->addItems(info.supportedCodecs());
        //ui->channelsCount->setMaximum(rec->getMaxChannelsCount());
    }
}
void MainWindow::refreshEnabledSources() {
    ui->refreshSources->setEnabled(false);
    ui->sourceFilePath->setEnabled(false);
    ui->sourcesList->setEnabled(false);
    ui->browseSourceFilePath->setEnabled(false);
    if (ui->sourceRadioDevice->isChecked()) {
        ui->sourcesList->setEnabled(true);
        ui->refreshSources->setEnabled(true);
    }
    else if (ui->sourceRadioFile->isChecked()) {
        ui->sourceFilePath->setEnabled(true);
        ui->browseSourceFilePath->setEnabled(true);
    }
}
void MainWindow::refreshEnabledDestinations() {
    ui->destinationFilePath->setEnabled(false);
    ui->destinationPathBrowse->setEnabled(false);
    ui->destinationTcpBufferDuration->setEnabled(false);
    ui->destinationTcpSocket->setEnabled(false);
    ui->destinationDeviceCombo->setEnabled(false);
    ui->refreshOutputDevices->setEnabled(false);
    ui->destinationPulseAudioLineEdit->setEnabled(false);

    if (ui->destinationDeviceRadio->isChecked()) {
        ui->destinationDeviceCombo->setEnabled(true);
        ui->refreshOutputDevices->setEnabled(true);
    }
    else if (ui->destinationRadioFile->isChecked()) {
        ui->destinationFilePath->setEnabled(true);
        ui->destinationPathBrowse->setEnabled(true);
    }
    else if (ui->destinationRadioTcp->isChecked()) {
        ui->destinationTcpSocket->setEnabled(true);
        ui->destinationTcpBufferDuration->setEnabled(true);
    }
#ifdef PULSE
    else if (ui->destinationRadioPulseAudio->isChecked()) {
        ui->destinationPulseAudioLineEdit->setEnabled(true);
    }
#endif

}
void MainWindow::tcpTargetConnected() {
    ui->statusBar->showMessage("Target connected",3000);
}
void MainWindow::refreshReadedData() {
    quint64 size = manager->getTransferedSize();
    int speed = size - lastReadedValue;
    ui->statusBar->showMessage("Readed data: " + wsize(size) + " - speed: " + wsize(speed) + "/s");

    lastReadedValue = size;
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

void MainWindow::on_refreshOutputDevices_clicked()
{
    ui->destinationDeviceCombo->clear();
    ui->destinationDeviceCombo->addItems(Manager::getDevicesNames(QAudio::AudioOutput));
}

void MainWindow::started() {
    setUserControlState(false);
    ui->pushButton->setText("Stop");
    lastReadedValue = 0;
    timer->start();
}
void MainWindow::refreshEstimatedBitrate() {
    const int bitrate = (ui->samplesRates->currentText().toInt() * ui->samplesSize->currentText().toInt() / 8) * ui->channelsCount->value();
    ui->statusBar->showMessage("estimated bitrate: " + wsize(bitrate) + "/s",2000);
}
void MainWindow::setUserControlState(const bool state) {
    ui->samplesRates->setEnabled(state);
    ui->samplesSize->setEnabled(state);
    ui->channelsCount->setEnabled(state);
    ui->codecList->setEnabled(state);

    ui->sourceRadioDevice->setEnabled(state);
    ui->sourcesList->setEnabled(state);
    ui->refreshSources->setEnabled(state);

    ui->sourceRadioFile->setEnabled(state);
    ui->sourceFilePath->setEnabled(state);
    ui->browseSourceFilePath->setEnabled(state);

    ui->destinationDeviceRadio->setEnabled(state);
    ui->destinationDeviceCombo->setEnabled(state);

    ui->destinationRadioFile->setEnabled(state);
    ui->destinationFilePath->setEnabled(state);
    ui->destinationTcpBufferDuration->setEnabled(state);
    ui->destinationRadioTcp->setEnabled(state);
    ui->destinationTcpSocket->setEnabled(state);
#ifdef PULSE
    ui->destinationRadioPulseAudio->setEnabled(state);
    ui->destinationPulseAudioLineEdit->setEnabled(state);
#endif
}
