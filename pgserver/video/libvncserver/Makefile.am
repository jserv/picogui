INCLUDES = -I$(top_builddir)/include -I$(top_srcdir)/include

noinst_LTLIBRARIES = libvncserver.la

libvncserver_la_SOURCES = \
	auth.c		\
	corre.c		\
	d3des.c		\
	hextile.c	\
	main.c		\
	rfbserver.c	\
	rre.c		\
	sockets.c	\
	sraRegion.c	\
	tight.c		\
	translate.c	\
	vncauth.c	\
	zlib.c		\
	httpd.c

noinst_HEADERS = \
	d3des.h		\
	keysym.h	\
	radon.h		\
	rfb.h		\
	rfbproto.h	\
	sraRegion.h

EXTRA_DIST = \
	tableinit24.c	\
	tableinitcmtemplate.c \
	tableinittctemplate.c \
	tabletrans24template.c \
	tabletranstemplate.c

libvncserver_la_DEPENDENCIES = \
	tableinit24.c	\
	tableinitcmtemplate.c \
	tableinittctemplate.c \
	tabletrans24template.c \
	tabletranstemplate.c

