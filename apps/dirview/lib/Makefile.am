lib_LTLIBRARIES = libpg_dirview.la

bin_PROGRAMS = test test_static

libpg_dirview_la_SOURCES = \
        libpg_dirview.h libpg_dirview.c debug.h \
	mime.h mime.c properties.h protocol.h protocol.c \
	proto_file.c displet.h

include_HEADERS = libpg_dirview.h debug.h protocol.h \
	mime.h properties.h displet.h

noinst_HEADERS = debug.h protocol.h mime.h properties.h \
	displet.h

test_SOURCES = test.c

test_LDADD = libpg_dirview.la

test_static_SOURCES = libpg_dirview.h libpg_dirview.c test.c \
	debug.h mime.h mime.c properties.h protocol.h protocol.c \
	proto_file.c displet.h


CFLAGS += @CFLAGS@ @PICOGUI_CFLAGS@ -Werror # @GLIB_CFLAGS@
LIBS   += @PICOGUI_LIBS@ # @GLIB_LIBS@
