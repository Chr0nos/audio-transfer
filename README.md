# Audio-Transfer
Audio streaming over LAN

this software let you stream audio from one or more client to a server connected to speakers with the local network

it use audio/pcm as default format to prevent cpu usage and keep quality

the bandwith consumtion for stereo 16bits 44.1khz is ~172kb/s

it can handle up to 9 channels, on 32bits at 320khz (depending of the hardwore on the server)

client and server are in the same application (they use the sames modules so why dont the job twice ?)

it's cross platform: win32 / linux / mac os and soon arm

## Build:

### Linux mint:

```
sudo aptitude install libpulse-dev build-essential \
 libqt5core5 libqt5gui5 libqt5network5 qtmultimedia5-dev \
 libqt5multimedia5-plugins libqt5multimedia5 git
git clone https://github.com/Chr0nos/audio-transfer.git
cd audio-transfer-client
/usr/lib/x86_64-linux-gnu/qt5/bin/qmake
make all clean
sudo cp audio-transfer-client /usr/bin/
```
    
### Linux Gentoo:
```
emerge -av dev-cvs/git
git clone https://github.com/Chr0nos/audio-transfer.git
cp -vr ./audio-transfer-client/gentoo/media-sound/audio-transfer-client \
  /usr/portage/media-sound/
emerge -av audio-transfer-client
```

### Windows

#### from sources
1. Install QtCreator (like: Qt 5.4.0 for Windows 32-bit MinGW 4.9.1 )
2. get https://github.com/Chr0nos/audio-transfer/archive/master.zip
unzip it where you want
3. load audio-transfer-client.pro with QtCreator
choose where you want compiled software
4. edit audio-transfer-client.pro and change the "DEFINES" use
you have to remove PORTAUDIO and PULSE
now you can build the project as release

---

#### get a binary

Just get a [binary](http://62.210.182.55/Chr0nos/audio-transfer.zip)
note: the build will not be recent as from source (i dont make a build on each commit)

