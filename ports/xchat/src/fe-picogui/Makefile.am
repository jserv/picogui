bin_PROGRAMS = xchat-picogui

xchat_picogui_LDADD = ../common/libxchatmain.a libpgui.a

if USE_MYGLIB
glibc = glib.c
endif

INCLUDES = -I$(includedir) -I/home/yann/pgui/cli_c/include	-DLOCALEDIR=\"$(datadir)/locale\"

xchat_picogui_SOURCES = fe-picogui.c $(glibc)
