bin_PROGRAMS = wclock

wclock_SOURCES = wclock.c map.c zones.c

includedir = @includedir@/picogui
include_HEADERS = wclock.h debug.h

LDFLAGS += @PICOGUI_LDFLAGS@
LDADD = @PICOGUI_LIBS@

CFLAGS = @CFLAGS@ @PICOGUI_CFLAGS@

CLEANFILES = map.inc

map.c: map.inc

map.inc: world-grey.pnm
	@if [ ! "`head -1 $<`" = "P5" ]; then \
	  echo >&2 "*** Error: $< is not of PNM P5 type"; \
	  exit 1; \
	fi
	rm -f $@
	echo "static int dims[] = {`head -3 $< | tail -1 | sed 's/ /,/g'`};" >> $@
	echo "static unsigned char map[] = {" >> $@
	od -vt x1 $(top_srcdir)/working/wclock/world-grey.pnm \
	| sed "s/^[^ ]*//g" \
	| sed "s/\([^ ][^ ]\)/0x\1,/g" \
	>> $@
	echo "};" >> $@

%.pbm: %.png
	pgmtopbm $< > $@

%.pbm: %.pnm
	pgmtopbm $< > $@

SUFFIXES = .pbm .pnm .png

