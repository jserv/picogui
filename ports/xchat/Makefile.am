## Process this file with automake to produce Makefile.in

SUBDIRS = po intl src

EXTRA_DIST = INSTALL.w32 FAQ xchat.png xchat.desktop xchat.spec doc/* \
				 config.h.w32 plugins/* scripts-perl/* scripts-python/*

if USE_GNOME

icon_DATA = xchat.png
icondir = $(datadir)/pixmaps

util_DATA = xchat.desktop
utildir = $(datadir)/gnome/apps/Internet

endif
