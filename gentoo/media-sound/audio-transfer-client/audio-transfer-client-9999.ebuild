EAPI="5"

inherit git-2 eutils qmake-utils

DESCRIPTION="Audio transfer client, to transfer audio over LAN"
HOMEPAGE="https://code.google.com/p/audio-transfer-client"

EGIT_REPO_URI="https://code.google.com/p/audio-transfer-client/"

LICENSE="LGPL-3"
SLOT="0"
KEYWORDS=""
IUSE="+qt4 qt5 pulseaudio"
REQUIRED_USE="^^ ( qt4 qt5 )"

RDEPEND="qt5? ( dev-qt/qtgui:5
                dev-qt/qtcore:5
                dev-qt/qtnetwork:5
                dev-qt/qtmultimedia:5 
	)
        qt4? ( dev-qt/qtgui:4
                dev-qt/qtcore:4
                dev-qt/qtmultimedia:4 
	)
	pulseaudio? ( 
		media-sound/pulseaudio 
	)"

DEPEND="${RDEPEND}"

src_configure() {
        use qt5 && eqmake5
        use qt4 && eqmake4
}

src_install() {
        dobin audio-transfer-client
}

