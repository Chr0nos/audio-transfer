build:
	Linux mint:
		sudo aptitude install libpulse-dev build-essential libqt5core5 libqt5gui5 libqt5network5 qtmultimedia5-dev libqt5multimedia5-plugins libqt5multimedia5 git
		git clone https://github.com/Chr0nos/audio-transfer.git
		cd audio-transfer-client
		/usr/lib/x86_64-linux-gnu/qt5/bin/qmake
		make all clean
		sudo cp audio-transfer-client /usr/bin/
		
	Linux Gentoo:
		emerge -av dev-cvs/git
		git clone https://github.com/Chr0nos/audio-transfer.git
		cp -vr ./audio-transfer-client/gentoo/media-sound/audio-transfer-client /usr/portage/media-sound/
		emerge -av audio-transfer-client
