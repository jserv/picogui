INCLUDES = -I$(top_builddir)/include -I$(top_srcdir)/include

noinst_LTLIBRARIES = libdebug.la

libdebug_la_SOURCES =

EXTRA_libdebug_la_SOURCES = videotest.c benchmark.c

libdebug_la_LIBADD       = $(DEBUGGING:%.c=%.lo)
libdebug_la_DEPENDENCIES = $(DEBUGGING:%.c=%.lo)
