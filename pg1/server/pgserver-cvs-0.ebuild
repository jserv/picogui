# Copyright 1999-2003 Gentoo Technologies, Inc.
# Distributed under the terms of the GNU General Public License v2

IUSE="fbcon gpm jpeg ncurses opengl png sdl svga truetype X directfb"

# ECVS_TOP_DIR="${PORTAGE_TMPDIR}"
ECVS_SERVER="cvs.sf.net:/cvsroot/pgui"
ECVS_MODULE="pgserver"
ECVS_CVS_OPTIONS="-dP"

inherit cvs
# inherit debug to enable debugging
inherit debug libtool

S=${WORKDIR}/${ECVS_MODULE}
DESCRIPTION="A new, scalable GUI (windowing) system"
HOMEPAGE="http://picogui.org"
SRC_URI=""

SLOT="0"
KEYWORDS="~x86"
LICENSE="LGPL-2"

# opengl libs need fixing
DEPEND="gpm? ( sys-libs/gpm )
	jpeg? ( media-libs/jpeg )
	ncurses? ( sys-libs/ncurses )
	opengl? ( media-libs/glut )
	png? ( media-libs/libpng )
	sdl? ( media-libs/libsdl )
	X? ( virtual/x11 )
	directfb? ( dev-libs/DirectFB )
	truetype? ( >=media-libs/freetype-2.1 )
	dev-util/cvs"
# do we support this?
#	sdl? ( truetype? ( media-libs/sdl-ttf ) )

src_compile() {
	cd ${S}
	# sometimes autogen fails due to ltmain - running it again works
	sh autogen.sh
	./configure
	python configscript/gentooconfig.py
	make
}

src_install () {
	einstall
	dodoc AUTHORS COPYING ChangeLog NEWS README*
}
