bin_PROGRAMS = gridgame
gridgame_SOURCES = gridgame.c ataxx.c
pkgdata_DATA = checkers.th

SUFFIXES += .th .ths .thp

%.ths: %.thp
	$(CPP) -x c -P -C -nostdinc -include gridth.h -o $@ $<

%.th: %.ths
	themec $<

LDFLAGS += @PICOGUI_LDFLAGS@
LDADD = @PICOGUI_LIBS@

CFLAGS = @CFLAGS@ @PICOGUI_CFLAGS@
