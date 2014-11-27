#!/bin/bash
./build.bash
useradd -s /dev/null -G audio audio-transfer
mkdir /etc/audio-transfer
cp ./server/server.ini /etc/audio-transfer/
chown -R audio-transfer:audio /etc/audio-transfer
chmod 664 /etc/audio-transfer/server.ini
cp audio-transfer-client /usr/bin/

