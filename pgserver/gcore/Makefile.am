INCLUDES = -I$(top_builddir)/include -I$(top_srcdir)/include

noinst_LIBRARIES = libgcore.a

libgcore_a_SOURCES = \
	grop.c 		 \
	render.c 	 \
	g_malloc.c 	 \
	div.c 		 \
	font.c 		 \
	handle.c 	 \
	g_error.c 	 \
	timer.c 	 \
	errortext.c      \
	driverinfo.c     \
	configfile.c     \
	signals.c	 \
	pgstring.c

EXTRA_DIST = inputdrivers.inc videodrivers.inc defaulttext.inc

libgcore_a_DEPENDENCIES = inputdrivers.inc videodrivers.inc


