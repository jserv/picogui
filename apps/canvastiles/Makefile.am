bin_PROGRAMS = canvastiles
canvastiles_SOURCES = canvastiles.c
pkgdata_DATA = tileset.th

SUFFIXES += .th .ths

%.th: %.ths
	themec $<

LDFLAGS += @PICOGUI_LDFLAGS@
LDADD = @PICOGUI_LIBS@

CFLAGS = @CFLAGS@ @PICOGUI_CFLAGS@
