INCLUDES = -I$(top_builddir)/include -I$(top_srcdir)/include

noinst_LIBRARIES = libdebug.a

libdebug_a_SOURCES = magicbutton.c

EXTRA_libdebug_a_SOURCES = videotest.c

libdebug_a_LIBADD       = $(DEBUGGING:%.c=%.o)
libdebug_a_DEPENDENCIES = $(DEBUGGING:%.c=%.o)
