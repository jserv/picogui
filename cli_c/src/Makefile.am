INCLUDES = -I$(top_srcdir)/include

lib_LTLIBRARIES = libpgui.la

libpgui_la_SOURCES = \
	api.c \
	netcore.c \
	pgfx.c \
	clientlib.h \
	pgfx_canvas.c \
	pgfx_bitmap.c \
	stddialog.c \
	$(PLATFORM)

EXTRA_libpgui_la_SOURCES = platform.c

libpgui_la_LIBADD       = $(PLATFORM:%.c=%.lo)
libpgui_la_DEPENDENCIES = $(PLATFORM:%.c=%.lo)
