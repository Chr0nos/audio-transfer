EAPI="4"

inherit git-2 eutils autotools

DESCRIPTION="Audio transfer client, to transfer audio over LAN"
HOMEPAGE="https://code.google.com/p/audio-transfer-server"

EGIT_REPO_URI="https://code.google.com/p/audio-transfer-client/"

LICENSE="LGPL-3"
SLOT="0"
KEYWORDS="~x86 ~amd64"

RDEPEND=">=dev-qt/qtgui-5
        >=dev-qt/qtcore-5
        >=dev-qt/qtnetwork-5
		>=dev-qt/qtmultimedia-5"
DEPEND="${RDEPEND}"

src_compile() {
        cd ${S}
        /usr/lib/qt5/bin/qmake -o Makefile
        emake || die "Failed to compile."
}

src_install() {
        emake install DESTDIR="${D}" || die "Failed to install properly."
        dobin audio-transfer-client
}
