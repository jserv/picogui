INCLUDES = -I$(top_builddir)/include -I$(top_srcdir)/include $(DRIVER_CFLAGS)

noinst_LTLIBRARIES = libinput.la

LIBS   = @LIBS@

libinput_la_SOURCES =		\
	input_drivers.c		\
	input_filters.c		\
	input_util.c		\
	cursor.c		\
	if_pntr_normalize.c	\
	if_key_dispatch.c	\
	if_magic.c		\
	if_hotspot.c		\
	if_key_preprocess.c	\
	if_pntr_dispatch.c	\
	if_pntr_preprocess.c	

EXTRA_libinput_la_SOURCES = \
	ncursesinput.c \
	gpm.c \
	r3912ts.c \
	sdlinput.c \
	svgainput.c \
	chipslicets.c \
	eventbroker.c \
	vr3ts.c \
	h3600ts.c \
	rrts.c \
	rr_sim.c \
	tuxts.c \
	ucb1x00.c \
	ttykb.c \
	remorakb.c \
	serialmouse.c \
	x11input.c \
	vncinput.c \
	mgl2input.c \
	adc7843.c \
	zaurus.c \
	rrts.c \
        ericsson_cb.c \
	ps2mouse.c \
	jsdev.c \
	rrttykb.c \
	rawttykb.c \
	if_touchscreen.c \
	if_client_adaptor.c \
	if_key_alpha.c \
	btkey.c

libinput_la_LIBADD       = $(INPUT:%.c=%.lo)
libinput_la_DEPENDENCIES = $(INPUT:%.c=%.lo)


