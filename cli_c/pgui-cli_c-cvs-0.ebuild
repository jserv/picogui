# Copyright 1999-2003 Gentoo Technologies, Inc.
# Distributed under the terms of the GNU General Public License v2

# ECVS_TOP_DIR="${PORTAGE_TMPDIR}"
ECVS_SERVER="cvs.sf.net:/cvsroot/pgui"
ECVS_MODULE="cli_c"
ECVS_CVS_OPTIONS="-dP"

inherit cvs
# inherit debug to enable debugging
inherit debug libtool

S=${WORKDIR}/${ECVS_MODULE}
DESCRIPTION="A new, scalable GUI (windowing) system - C client library"
HOMEPAGE="http://picogui.org"
SRC_URI=""

SLOT="0"
KEYWORDS="~x86"
LICENSE="LGPL-2"

DEPEND="pgui-base/pgserver-cvs
	dev-util/cvs"

src_compile() {
	cd ${S}
	sh autogen.sh
	./configure
	make
}

src_install () {
	einstall
	dodoc AUTHORS COPYING ChangeLog NEWS README*
}
