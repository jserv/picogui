## Process this file with automake to produce Makefile.in

noinst_LIBRARIES = libxchatmain.a

INCLUDES = -I$(includedir)

EXTRA_DIST = makefile.msc \
	cfgfiles.h \
	ctcp.h \
	dcc.h \
	fe.h \
	history.h \
	identd.c \
	ignore.h \
	inbound.h \
	inet.h \
	modes.h \
	network.h \
	notify.h \
	outbound.h \
	perl.c \
	perlc.h \
	plugin.h \
	pys_signals.h \
	python.c \
	pythonc.h \
	pysig.py \
	server.h \
	ssl.h \
	ssl.c	\
	text.h \
	userlist.h \
	util.h \
	xchat.h \
	xchatc.h \
	jcode.c \
	jcode.h

if DO_PERL
perl_c = perl.c
endif

if DO_PYTHON
python_c = python.c
endif

if USE_OPENSSL
ssl_c = ssl.c
endif

if DO_ICONV
jcode_c = jcode.c
endif

libxchatmain_a_SOURCES = cfgfiles.c ctcp.c dcc.c \
      history.c ignore.c inbound.c $(jcode_c) modes.c network.c \
		notify.c outbound.c $(perl_c) plugin.c $(python_c) server.c \
		$(ssl_c) text.c userlist.c util.c xchat.c
