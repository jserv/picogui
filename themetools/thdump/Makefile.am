bin_PROGRAMS = thdump

thdump_SOURCES = symtab.c main.c thdump.h

INCLUDES = -I$(top_srcdir)/include

CONSTANTS := $(top_srcdir)/include/picogui/constants.h
MKSYMTAB  := perl mksymtab.pl

symtab.c : $(CONSTANTS) mksymtab.pl
	$(MKSYMTAB) < $(CONSTANTS) > symtab.c

DISTCLEANFILES = symtab.c

EXTRA_DIST = mksymtab.pl

