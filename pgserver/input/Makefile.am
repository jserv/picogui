INCLUDES = -I$(top_builddir)/include -I$(top_srcdir)/include $(DRIVER_CFLAGS)

noinst_LIBRARIES = libinput.a

libinput_a_SOURCES = \
    $(INPUT)

EXTRA_libinput_a_SOURCES = \
	ncursesinput.c \
	r3912ts.c \
	sdlinput.c \
	svgainput.c \
	tsinput.c

libinput_a_LIBADD       = $(INPUT:%.c=%.o)
libinput_a_DEPENDENCIES = $(INPUT:%.c=%.o)


