bin_PROGRAMS = edemo

edemo_SOURCES = edemo.c special.c themebar.c canvastst.c chaos_fire.c dialogdemo.c pterm.c menutest.c appbar.c

LDFLAGS += @PICOGUI_LDFLAGS@
LDADD = @PICOGUI_LIBS@ -lcrypt -lm

CFLAGS = @CFLAGS@ @PICOGUI_CFLAGS@ -g -Wall
