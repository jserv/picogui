bin_PROGRAMS = xchat-picogui

xchat_picogui_LDADD = ../common/libxchatmain.a -lpgui

if USE_MYGLIB
glibc = glib.c
endif

INCLUDES = -I$(includedir) -DLOCALEDIR=\"$(datadir)/locale\"

xchat_picogui_SOURCES = fe-picogui.c output.c $(glibc)
