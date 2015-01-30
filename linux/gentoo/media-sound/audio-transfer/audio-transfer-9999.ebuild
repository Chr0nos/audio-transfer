EAPI="5"

inherit git-2 eutils qmake-utils flag-o-matic user

DESCRIPTION="Audio transfer client, to transfer audio over LAN"
HOMEPAGE="https://github.com/Chr0nos/audio-transfer"

EGIT_REPO_URI="https://github.com/Chr0nos/audio-transfer.git"

LICENSE="GPL-3"
SLOT="0"
KEYWORDS=""
IUSE="+qt4 qt5 pulseaudio debug portaudio +comline +server systemd"
REQUIRED_USE="^^ ( qt4 qt5 )"
append-cppflags "-DMULTIMEDIA"

RDEPEND="qt5? ( dev-qt/qtgui:5
                dev-qt/qtcore:5
                dev-qt/qtnetwork:5
                dev-qt/qtmultimedia:5
		dev-qt/qtwidgets:5
	)
        qt4? ( dev-qt/qtgui:4
                dev-qt/qtcore:4
                dev-qt/qtmultimedia:4
	)
	pulseaudio? (
		media-sound/pulseaudio
	)
	portaudio? (
		media-libs/portaudio
	)
	systemd? (
		sys-apps/systemd
	)"

DEPEND="${RDEPEND}"

src_configure() {
	use comline && append-cppflags "-DCOMLINE"
	use pulseaudio && append-cppflags "-DPULSEAUDIO"
	use portaudio && append-cppflags "-DPORTAUDIO"
	use debug && append-cppflags "-DDEBUG"
	use server && append-cppflags "-DSERVER"


	#dont works if i put this in a src_compile() section
	use qt5 && eqmake5
	use qt4 && eqmake4
}
pkg_preinst() {
	if use systemd; then
		enewuser audio-transfer -1 /dev/null /home/audio-transfer audio
		dodir /home/audio-transfer
		fowners audio-transfer /home/audio-transfer
                fowners audio-transfer /etc/audio-transfer/
                fowners audio-transfer /etc/audio-transfer/server.ini
	fi
}
src_install() {
	if use server; then
		insinto /etc/audio-transfer
		doins ./server/server.ini
		if use systemd; then
			insinto /usr/lib/systemd/system/
			doins ./server/audio-transfer-server.service
		fi
	fi
	insinto /usr/bin
	dobin audio-transfer
}

