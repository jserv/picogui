INCLUDES = -I$(top_builddir)/include -I$(top_srcdir)/include

noinst_LTLIBRARIES = libgcore.la

libgcore_la_SOURCES = \
	grop.c 		 \
	render.c 	 \
	g_malloc.c 	 \
	div.c 		 \
	handle.c 	 \
	g_error.c 	 \
	timer.c 	 \
	errortext.c      \
	driverinfo.c     \
	configfile.c     \
	pgstring.c       \
	globals.c        \
	init.c           \
	requests.c

EXTRA_DIST = inputdrivers.inc videodrivers.inc defaulttext.inc requests.inc

libgcore_la_DEPENDENCIES = inputdrivers.inc videodrivers.inc defaulttext.inc requests.inc
