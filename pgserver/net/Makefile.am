INCLUDES = -I$(top_builddir)/include -I$(top_srcdir)/include

noinst_LIBRARIES = libnet.a

libnet_a_SOURCES = \
	dispatch.c  	\
	eventq.c	\
	request.c

EXTRA_DIST = requests.inc

libnet_a_DEPENDENCIES = requests.inc


