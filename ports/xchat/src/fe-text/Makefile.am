bin_PROGRAMS = xchat-text

xchat_text_LDADD = ../common/libxchatmain.a

if USE_MYGLIB
glibc = glib.c
endif

INCLUDES = -I$(includedir)	-DLOCALEDIR=\"$(datadir)/locale\"

xchat_text_SOURCES = fe-text.c $(glibc)
