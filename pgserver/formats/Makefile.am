INCLUDES = -I$(top_builddir)/include -I$(top_srcdir)/include

noinst_LIBRARIES = libformats.a 

libformats_a_SOURCES = plaintext.c

EXTRA_libformats_a_SOURCES = pnm.c jpeg.c bmp.c html.c

libformats_a_LIBADD       = $(FORMATS:%.c=%.o)
libformats_a_DEPENDENCIES = $(FORMATS:%.c=%.o)

