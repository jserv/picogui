INCLUDES = -I$(top_builddir)/include -I$(top_srcdir)/include

noinst_LIBRARIES = libvidwrap.a

libvidwrap_a_SOURCES =

EXTRA_libvidwrap_a_SOURCES = rotate90.c rotate180.c

libvidwrap_a_LIBADD       = $(VIDWRAP:%.c=%.o)
libvidwrap_a_DEPENDENCIES = $(VIDWRAP:%.c=%.o)
