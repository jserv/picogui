INCLUDES = -I$(top_builddir)/include -I$(top_srcdir)/include $(DRIVER_CFLAGS)

noinst_LTLIBRARIES = libformats.la 

libformats_la_SOURCES =

EXTRA_libformats_la_SOURCES = pnm.c jpeg.c bmp.c html.c png.c gif.c plaintext.c

libformats_la_LIBADD       = $(FORMATS:%.c=%.lo)
libformats_la_DEPENDENCIES = $(FORMATS:%.c=%.lo)
