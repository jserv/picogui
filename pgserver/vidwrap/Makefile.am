INCLUDES = -I$(top_builddir)/include -I$(top_srcdir)/include

noinst_LTLIBRARIES = libvidwrap.la

libvidwrap_la_SOURCES =

EXTRA_libvidwrap_la_SOURCES = rotate90.c rotate180.c rotate270.c

libvidwrap_la_LIBADD       = $(VIDWRAP:%.c=%.lo)
libvidwrap_la_DEPENDENCIES = $(VIDWRAP:%.c=%.lo)
