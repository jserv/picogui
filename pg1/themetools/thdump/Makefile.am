bin_PROGRAMS = thdump

thdump_SOURCES = symtab.c main.c thdump.h

CONSTANTS := $(PGSERVER_INCLUDES)/picogui/constants.h
MKSYMTAB  := perl $(srcdir)/mksymtab.pl

symtab.c : $(CONSTANTS) $(srcdir)/mksymtab.pl
	$(MKSYMTAB) < $(CONSTANTS) > symtab.c

DISTCLEANFILES = symtab.c

INCLUDES = $(PGSERVER_CFLAGS)

EXTRA_DIST = $(srcdir)/mksymtab.pl

