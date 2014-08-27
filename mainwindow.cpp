#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "manager.h"
#include "readini.h"
#include "size.h"
#include "graphicgenerator.h"

#include <QtGui>
#include <QString>
#include <QtMultimedia/QAudioOutput>
#include <QFileDialog>
#include <QDialog>
#include <QTimer>
#include <QDesktopWidget>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    moveToCenter();
    modeSource = Manager::Device;
    modeDest = Manager::Tcp;

    manager = new Manager(this);

    timer = new QTimer(this);
    timer->setInterval(300);
    connect(manager,SIGNAL(stoped()),this,SLOT(recStoped()));
    on_refreshSources_clicked();
    ui->destinationDeviceCombo->addItems(Manager::getDevicesNames(QAudio::AudioOutput));

    connect(ui->sourceRadioDevice,SIGNAL(clicked()),this,SLOT(refreshEnabledSources()));
    connect(ui->sourceRadioFile,SIGNAL(clicked()),this,SLOT(refreshEnabledSources()));
    connect(ui->sourceRadioZeroDevice,SIGNAL(clicked()),this,SLOT(refreshEnabledSources()));
    connect(ui->sourceRadioPulseAudio,SIGNAL(clicked()),this,SLOT(refreshEnabledSources()));
    connect(ui->sourceRadioPortAudio,SIGNAL(clicked()),this,SLOT(refreshEnabledSources()));
    connect(ui->destinationDeviceRadio,SIGNAL(clicked()),this,SLOT(refreshEnabledDestinations()));
    connect(ui->destinationRadioFile,SIGNAL(clicked()),this,SLOT(refreshEnabledDestinations()));
    connect(ui->destinationRadioTcp,SIGNAL(clicked()),this,SLOT(refreshEnabledDestinations()));
    connect(ui->destinationRadioPulseAudio,SIGNAL(clicked()),SLOT(refreshEnabledDestinations()));
    connect(ui->destinationRadioZeroDevice,SIGNAL(clicked()),this,SLOT(refreshEnabledDestinations()));
    connect(ui->destinationRadioPortAudio,SIGNAL(clicked()),this,SLOT(refreshEnabledDestinations()));
    connect(ui->checkboxSourceOutput,SIGNAL(clicked()),this,SLOT(on_refreshSources_clicked()));

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
    ui->sourceRadioPulseAudio->setEnabled(false);
    ui->sourceRadioPulseAudio->deleteLater();
#endif
#ifndef PORTAUDIO
    ui->sourceRadioPortAudio->setEnabled(false);
    ui->sourceRadioPortAudio->hide();
    ui->destinationRadioPortAudio->setEnabled(false);
    ui->destinationRadioPortAudio->hide();
    ui->destinationPortAudioList->hide();
    ui->refreshPortAudioDestinationButton->hide();
    ui->portAudioSourceList->setEnabled(false);
    ui->portAudioSourceList->hide();
    ui->portAudioRefreshButton->hide();
#endif
    debug("configuration path: " + getConfigFilePath());

    this->ini = new Readini(getConfigFilePath(),this);
    if (ini->exists()) {
        configLoad(ini);
        if (ini->getValue("general","auto-start") == "1") on_pushButton_clicked();
    }
    //set the window to the minimal height
    this->resize(this->geometry().width(),1);
    speeds << 1 << 100 << 50;
    refreshGraphics();
}

MainWindow::~MainWindow()
{
    delete manager;
    delete ini;
    delete ui;
}
void MainWindow::errors(const QString error) {
    ui->statusBar->showMessage(error,3000);
    this->debug(error);
}
void MainWindow::debug(const QString message) {
    ui->debug->addItem(message);
    ui->debug->scrollToBottom();
}

void MainWindow::on_refreshSources_clicked()
{
    ui->sourcesList->clear();
    ui->sourcesList->addItems(Manager::getDevicesNames(QAudio::AudioInput));
    if (ui->checkboxSourceOutput->isChecked()) {
        ui->sourcesList->addItems(Manager::getDevicesNames(QAudio::AudioOutput));
    }
}


void MainWindow::on_pushButton_clicked()
{
    //record button.
    if (!manager->isRecording()) {
        ui->debug->clear();
        const int bitrate = (ui->samplesRates->currentText().toInt() * ui->samplesSize->currentText().toInt() / 8) * ui->channelsCount->value();
        debug("estimated bitrate: " + Size::getWsize(bitrate));
        Manager::userConfig mc;
        mc.modeInput = Manager::None;
        mc.modeOutput = Manager::None;

        mc.format = new AudioFormat();
        mc.format->setCodec(ui->codecList->currentText());
        mc.format->setSampleRate(ui->samplesRates->currentText().toInt());
        mc.format->setSampleSize(ui->samplesSize->currentText().toInt());
        mc.format->setChannelCount(ui->channelsCount->value());

        mc.filePathOutput = ui->destinationFilePath->text();
        mc.filePathInput = ui->sourceFilePath->text();
        mc.devices.input = ui->sourcesList->currentIndex();
        mc.devices.output = ui->destinationDeviceCombo->currentIndex();
        mc.bufferSize = bitrate * ui->destinationTcpBufferDuration->value() / 1000 / 100;
        mc.bufferMaxSize = 2097152; //2Mb

#ifdef PORTAUDIO
        mc.portAudio.deviceIdOutput = ui->destinationPortAudioList->currentIndex();
        mc.portAudio.deviceIdInput = ui->portAudioSourceList->currentIndex();
#endif
        debug("buffer size: " + Size::getWsize(mc.bufferSize));

        //SOURCES
        if (ui->sourceRadioFile->isChecked()) mc.modeInput = Manager::File;
        else if (ui->sourceRadioDevice->isChecked()) mc.modeInput = Manager::Device;
        else if (ui->sourceRadioZeroDevice->isChecked()) mc.modeInput = Manager::Zero;
        else if (ui->sourceRadioPulseAudio->isChecked()) mc.modeInput = Manager::PulseAudio;
        else if (ui->sourceRadioPortAudio->isChecked()) mc.modeInput = Manager::PortAudio;

        //DESTINATIONS
        if (ui->destinationRadioFile->isChecked()) {
            mc.modeOutput = Manager::File;
        }

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
            //mc.modeOutput = Manager::PulseAudioAsync;

        }
        else if (ui->destinationRadioZeroDevice->isChecked()) mc.modeOutput = Manager::Zero;
        else if (ui->destinationRadioPortAudio->isChecked()) mc.modeOutput = Manager::PortAudio;

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
        QList<QAudioDeviceInfo> infoList = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);
        if (ui->checkboxSourceOutput->isChecked()) {
            infoList << QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);
        }

        QAudioDeviceInfo info = infoList.at(ui->sourcesList->currentIndex());
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
        QList<QAudioDeviceInfo> infoList = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);
        infoList.append(QAudioDeviceInfo::availableDevices(QAudio::AudioOutput));

        QAudioDeviceInfo info = infoList.at(ui->sourcesList->currentIndex());

        ui->samplesRates->addItems(Manager::intListToQStringList(info.supportedSampleRates()));
        ui->codecList->addItems(info.supportedCodecs());
        //ui->channelsCount->setMaximum(rec->getMaxChannelsCount());
    }
}
void MainWindow::refreshEnabledSources() {
    ui->refreshSources->setEnabled(false);
    ui->sourceFilePath->setEnabled(false);
    ui->sourcesList->setEnabled(false);
    ui->portAudioSourceList->setEnabled(false);
    ui->portAudioRefreshButton->setEnabled(false);

    ui->browseSourceFilePath->setEnabled(false);
    if (ui->sourceRadioDevice->isChecked()) {
        ui->sourcesList->setEnabled(true);
        ui->refreshSources->setEnabled(true);
        modeSource = Manager::Device;
    }
    else if (ui->sourceRadioFile->isChecked()) {
        ui->sourceFilePath->setEnabled(true);
        ui->browseSourceFilePath->setEnabled(true);
        modeSource = Manager::File;
    }
    else if (ui->sourceRadioZeroDevice->isChecked()) modeSource = Manager::Zero;
#ifdef PULSE
    else if (ui->sourceRadioPulseAudio->isChecked()) modeSource = Manager::PulseAudio;
#endif
#ifdef PORTAUDIO
    else if (ui->sourceRadioPortAudio->isChecked()) {
        modeSource = Manager::PortAudio;
        ui->portAudioSourceList->setEnabled(true);
        ui->portAudioRefreshButton->setEnabled(true);
    }
#endif
}
void MainWindow::refreshEnabledDestinations() {
    ui->destinationFilePath->setEnabled(false);
    ui->destinationPathBrowse->setEnabled(false);
    ui->destinationTcpBufferDuration->setEnabled(false);
    ui->destinationTcpSocket->setEnabled(false);
    ui->destinationDeviceCombo->setEnabled(false);
    ui->refreshOutputDevices->setEnabled(false);
    //port audio
    ui->destinationPortAudioList->setEnabled(false);
    ui->refreshPortAudioDestinationButton->setEnabled(false);

#ifdef PULSE
    ui->destinationPulseAudioLineEdit->setEnabled(false);
#endif

    if (ui->destinationDeviceRadio->isChecked()) {
        ui->destinationDeviceCombo->setEnabled(true);
        ui->refreshOutputDevices->setEnabled(true);
        modeDest = Manager::Device;
    }
    else if (ui->destinationRadioFile->isChecked()) {
        ui->destinationFilePath->setEnabled(true);
        ui->destinationPathBrowse->setEnabled(true);
        modeDest = Manager::File;
    }
    else if (ui->destinationRadioTcp->isChecked()) {
        ui->destinationTcpSocket->setEnabled(true);
        ui->destinationTcpBufferDuration->setEnabled(true);
        modeDest = Manager::Tcp;
    }
#ifdef PULSE
    else if (ui->destinationRadioPulseAudio->isChecked()) {
        ui->destinationPulseAudioLineEdit->setEnabled(true);
        modeDest = Manager::PulseAudio;
    }
#endif
    else if (ui->destinationRadioZeroDevice->isChecked()) modeDest = Manager::Zero;
    else if (ui->destinationRadioPortAudio->isChecked()) {
        modeDest = Manager::PortAudio;
        ui->destinationPortAudioList->setEnabled(true);
        ui->refreshPortAudioDestinationButton->setEnabled(true);
    }

}
void MainWindow::refreshReadedData() {
    const quint64 size = manager->getTransferedSize();
    const int speed = size - lastReadedValue;
    ui->statusBar->showMessage("Readed data: " + Size::getWsize(size) + " - speed: " + Size::getWsize(speed) + "/s");

    lastReadedValue = size;

    speeds.append(speed);
    if (speeds.count() > 50) speeds.removeAt(0);
    refreshGraphics();
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
    ui->statusBar->showMessage("estimated bitrate: " + Size::getWsize(this->getBitrate()) + "/s",2000);
}
int MainWindow::getBitrate() {
    return (ui->samplesRates->currentText().toInt() * ui->samplesSize->currentText().toInt() / 8) * ui->channelsCount->value();
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

    ui->sourceRadioZeroDevice->setEnabled(state);

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
    ui->sourceRadioPulseAudio->setEnabled(state);
#endif
#ifdef PORTAUDIO
        ui->sourceRadioPortAudio->setEnabled(state);
        ui->destinationRadioPortAudio->setEnabled(state);
        ui->portAudioRefreshButton->setEnabled(state);
        ui->destinationPortAudioList->setEnabled(state);
        ui->portAudioSourceList->setEnabled(state);
#endif
    ui->destinationRadioZeroDevice->setEnabled(state);

    ui->checkboxSourceOutput->setEnabled(state);
}

QString MainWindow::getConfigFilePath() {
    return QDir::homePath() + "/.audio-transfer-client.ini";
}

void MainWindow::on_configSave_clicked()
{
    configSave(this->ini);
}
void MainWindow::configSave(Readini *ini) {
    ini->open(QIODevice::WriteOnly);
    if (!ini->isWritable()) {
        errors("cannot write in the configuration file: check perms");
        return;
    }
    ui->configSave->setEnabled(false);
    ini->setValue("format","codec",ui->codecList->currentText());
    ini->setValue("format","sampleSize",ui->samplesSize->currentText());
    ini->setValue("format","sampleRate",ui->samplesRates->currentText());
    ini->setValue("format","channels",ui->channelsCount->value());

    ini->setValue("source","device",ui->sourcesList->currentText());
    ini->setValue("source","mode",modeSource);
    ini->setValue("source","file",ui->sourceFilePath->text());

    ini->setValue("target","mode",modeDest);
    ini->setValue("target","file",ui->destinationFilePath->text());
    ini->setValue("target","tcp",ui->destinationTcpSocket->text());
    ini->setValue("target","tcpBuffer",ui->destinationTcpBufferDuration->value());
#ifdef PULSE
    ini->setValue("target","pulse",ui->destinationPulseAudioLineEdit->text());
#endif
#ifdef PORTAUDIO
    ini->setValue("target","portaudio",ui->destinationPortAudioList->currentText());
    ini->setValue("source","portaudio",ui->portAudioSourceList->currentText());
#endif

    ini->setValue("target","device",ui->destinationDeviceCombo->currentText());
    ini->setValue("options","sourceOutput",ui->checkboxSourceOutput->isChecked());

    //write all of this inside the .ini file
    ini->flush();
    debug("configuration saved");
    ui->statusBar->showMessage("configuration saved",3000);
    ui->configSave->setEnabled(true);
}
void MainWindow::configLoad(Readini *ini) {
    ui->configSave->setEnabled(false);
    if (!ini->exists()) {
        ui->configSave->setEnabled(true);
        return;
    }
    ui->checkboxSourceOutput->setChecked(intToBool(ini->getValue("options","sourceOutput").toInt()));

    const int deviceIdSource = ui->sourcesList->findText(ini->getValue("source","device"));
    if (deviceIdSource) ui->sourcesList->setCurrentIndex(deviceIdSource);

    const int codecId = ui->codecList->findText(ini->getValue("format","codec"));
    if (codecId) ui->codecList->setCurrentIndex(codecId);

    const int sampleSizePos = ui->samplesSize->findText(ini->getValue("format","sampleSize"));
    if (sampleSizePos) ui->samplesSize->setCurrentIndex(sampleSizePos);

    const int sampleRatePos = ui->samplesRates->findText(ini->getValue("format","sampleRate"));
    if (sampleRatePos) ui->samplesRates->setCurrentIndex(sampleRatePos);

    const int channels = ini->getValue("format","channels").toInt();
    if ((channels) && (channels <= ui->channelsCount->maximum())) ui->channelsCount->setValue(channels);

    const int deviceIdTarget = ui->destinationDeviceCombo->findText(ini->getValue("target","device"));
    if (deviceIdTarget) ui->destinationDeviceCombo->setCurrentIndex(deviceIdTarget);


    ui->sourceFilePath->setText(ini->getValue("source","file"));


#ifdef PULSE
    ui->destinationPulseAudioLineEdit->setText(ini->getValue("target","pulse"));
#endif
#ifdef PORTAUDIO
    ui->destinationPortAudioList->addItem(ini->getValue("target","portaudio"));
    ui->destinationPortAudioList->setCurrentIndex(ui->destinationPortAudioList->count() -1);
#endif

    ui->destinationFilePath->setText(ini->getValue("target","file"));
    ui->destinationTcpSocket->setText(ini->getValue("target","tcp"));
    ui->destinationTcpBufferDuration->setValue(ini->getValue("target","tcpBuffer").toInt());
    switch (ini->getValue("source","mode").toInt()) {
        case Manager::Device:
            ui->sourceRadioDevice->setChecked(true);
            break;
        case Manager::File:
            ui->sourceRadioFile->setChecked(true);
            break;
        case Manager::Zero:
            ui->sourceRadioZeroDevice->setChecked(true);
            break;
#ifdef PULSE
        case Manager::PulseAudio:
            ui->sourceRadioPulseAudio->setChecked(true);
            break;
#endif
        default:
            debug("config: ignored source mode");
            break;
    }
    refreshEnabledSources();

    switch (ini->getValue("target","mode").toInt()) {
        case Manager::Device:
            ui->destinationDeviceRadio->setChecked(true);
            break;
        case Manager::Tcp:
            ui->destinationRadioTcp->setChecked(true);
            break;
        case Manager::File:
            ui->destinationRadioFile->setChecked(true);
            break;
#ifdef PULSE
        case Manager::PulseAudio:
            ui->destinationRadioPulseAudio->setChecked(true);
            break;
#endif
        case Manager::Zero:
            ui->destinationRadioZeroDevice->setChecked(true);
            break;
        default:
            debug("config: ignored target mode");
            break;
    }
    refreshEnabledDestinations();
    ui->configSave->setEnabled(true);
}
void MainWindow::moveToCenter() {
    const QRect screen = QApplication::desktop()->screenGeometry();
    this->move(screen.center() - this->rect().center());
}
bool MainWindow::intToBool(const int value) {
    if (value) return true;
    else return false;
}

void MainWindow::refreshPortAudioDevices(QComboBox *target) {
#ifdef PORTAUDIO
    PortAudioDevice api(NULL,this); //the NULL is ok here because we dont need an AudioFormat to get devices list
    target->clear();
    target->addItems(api.getDevicesNames());
    target->setCurrentIndex(target->count() -1);
#endif
}

void MainWindow::on_portAudioRefreshButton_clicked() {
    refreshPortAudioDevices(ui->portAudioSourceList);
}

void MainWindow::on_refreshPortAudioDestinationButton_clicked()
{
    refreshPortAudioDevices(ui->destinationPortAudioList);
}
void MainWindow::setDefaultFormats() {
    ui->codecList->clear();
    ui->codecList->addItem("audio/pcm");

    QStringList samplesRates;
    samplesRates << "8000" << "11024" << "22025" << "44100" << "48000" << "96000" << "196000" << "320000";
    ui->samplesRates->clear();
    ui->samplesRates->addItems(samplesRates);

    QStringList samplesSize;
    samplesSize << "8" << "16" << "24" << "32";
    ui->samplesSize->clear();
    ui->samplesSize->addItems(samplesSize);
}

void MainWindow::refreshGraphics() {
    if (!ui->checkShowStreamSpeed->isChecked()) return;
    GraphicGenerator g(&speeds,ui->graphicViewLabel,this);
    g.refresh();

}

void MainWindow::on_buttonResetGraphic_clicked()
{
    this->speeds.clear();
    refreshGraphics();
}

void MainWindow::on_buttonSaveGraphic_clicked()
{
    if (!ui->graphicViewLabel->pixmap()) {
        debug("cannot save graphic: no data to save, use the checkbox in option and to a transfer before");
        return;
    }

    QFileDialog d(this);
    d.setAcceptMode(QFileDialog::AcceptSave);
    d.setNameFilter("*.jpg");
    d.exec();
    d.show();
    if (d.selectedFiles().isEmpty()) {
        debug("no file selected: abording");
        return;
    }
    QString fileName = d.selectedFiles().at(0);
    if ((fileName.length() < 4) || (fileName.right(4) != ".jpg")) fileName.append(".jpg");

    ui->graphicViewLabel->pixmap()->toImage().save(fileName);
    debug("graphic saved to " + fileName);
}
