INCLUDES = -I$(top_builddir)/include -I$(top_srcdir)/include

noinst_LTLIBRARIES = libnet.la

libnet_la_SOURCES = \
	dispatch.c  	\
	eventq.c	\
	request.c	\
	mainloop.c

EXTRA_DIST = requests.inc

libnet_la_DEPENDENCIES = requests.inc


