INCLUDES = -I$(top_builddir)/include -I$(top_srcdir)/include

noinst_LTLIBRARIES = libos.la

libos_la_SOURCES = \
	$(OS)

EXTRA_libos_la_SOURCES = \
	posix.c			\
	uclinux.c		\
	posix_signals.c		\
	posix_commandline.c


libos_la_LIBADD       = $(OS:%.c=%.lo)
libos_la_DEPENDENCIES = $(OS:%.c=%.lo)
