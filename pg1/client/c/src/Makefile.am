INCLUDES = -I$(top_srcdir)/include $(PGSERVER_CFLAGS)

keynames.c: $(top_srcdir)/../../server/include/picogui/pgkeys.h
	(cd $(top_srcdir); python grab_keynames.py ; cd -; mv $(top_srcdir)/keynames.c .)

lib_LTLIBRARIES = libpgui.la

libpgui_la_SOURCES = \
	api.c \
	netcore.c \
	pgfx.c \
	clientlib.h \
	pgfx_canvas.c \
	pgfx_bitmap.c \
	stddialog.c \
	dlg_datepicker.c \
	dlg_fontpicker.c \
	dlg_filepicker.c \
	dlg_colorpicker.c \
	applet.c \
	keynames.c \
	$(PLATFORM)

EXTRA_libpgui_la_SOURCES = platform.c

libpgui_la_LIBADD       = $(PLATFORM:%.c=%.lo)
libpgui_la_DEPENDENCIES = $(PLATFORM:%.c=%.lo)
