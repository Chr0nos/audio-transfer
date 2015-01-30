#!/bin/bash
#if you are on debian like, just fetch this script
sudo aptitude install libpulse-dev build-essential libqt5core5 libqt5gui5 libqt5network5 qtmultimedia5-dev libqt5multimedia5-plugins libqt5multimedia5 git
git clone https://github.com/Chr0nos/audio-transfer.git
cd audio-transfer-client
/usr/lib/x86_64-linux-gnu/qt5/bin/qmake
make all clean
//uncomment this like if you wan to use systemd script
#sudo useradd -s /dev/null -G audio audio-transfer
#sudo mkdir /etc/audio-transfer
#sudo cp ./server/server.ini /etc/audio-transfer/
#sudo chown -R audio-transfer:audio /etc/audio-transfer
#sudo chmod 664 /etc/audio-transfer/server.ini
sudo cp audio-transfer-client /usr/bin/
