INCLUDES = -I$(top_builddir)/include -I$(top_srcdir)/include $(DRIVER_CFLAGS)

noinst_LIBRARIES = libinput.a

libinput_a_SOURCES = \
    $(INPUT)

EXTRA_libinput_a_SOURCES = \
	ncursesinput.c \
	gpm.c \
	r3912ts.c \
	sdlinput.c \
	svgainput.c \
	chipslicets.c \
	eventbroker.c \
	vr3ts.c \
	tuxts.c \
	ucb1x00.c \
	ttykb.c \
	remorakb.c \
	serialmouse.c \
	x11input.c
	mgl2input.c

libinput_a_LIBADD       = $(INPUT:%.c=%.o)
libinput_a_DEPENDENCIES = $(INPUT:%.c=%.o)


