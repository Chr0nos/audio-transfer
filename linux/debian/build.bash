#!/bin/bash
#if you are on debian like, just fetch this script
sudo aptitude install libpulse-dev build-essential libqt5core5 libqt5gui5 libqt5network5 qtmultimedia5-dev git
git clone https://code.google.com/p/audio-transfer-client
cd audio-transfer-client
/usr/lib/x86_64-linux-gnu/qt5/bin/qmake
make all clean
sudo cp audio-transfer-client /usr/bin/
