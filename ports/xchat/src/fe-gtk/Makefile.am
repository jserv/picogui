bin_PROGRAMS = xchat

CFLAGS = $(GUI_CFLAGS)
LIBS = $(GUI_LIBS)

INCLUDES = -I$(includedir)

xchat_LDADD = ../common/libxchatmain.a

EXTRA_DIST = makefile.msc \
	about.h ascii.h banlist.h chanlist.h dialog.h editlist.h fake_gnome.h \
	fe-gtk.h fkeys.h gtkutil.h maingui.h menu.h mmx_cmod.S mmx_cmod.h \
	notifygui.h palette.h panel.c panel.h pixmaps.h plugingui.c rawlog.h \
	search.h serverlist.h settings.h textgui.h urlgrab.h userlistgui.h \
	usermenu.h wins.h xtext.h

if USE_MMX
mmx_cmod_S = mmx_cmod.S
endif

if DO_PANEL
panel_c = panel.c
endif

if DO_PLUGIN
plugingui_c = plugingui.c
endif

xchat_SOURCES = about.c ascii.c banlist.c chanlist.c dccgui.c dialog.c \
			editlist.c fe-gtk.c fkeys.c gtkutil.c ignoregui.c \
			maingui.c menu.c $(mmx_cmod_S) notifygui.c palette.c $(panel_c) \
			pixmaps.c $(plugingui_c) rawlog.c search.c serverlist.c settings.c \
			textgui.c usermenu.c urlgrab.c userlistgui.c wins.c xtext.c
