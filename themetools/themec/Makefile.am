bin_PROGRAMS = themec

themec_SOURCES = pgtheme.y pglex.l main.c backend.c symtab.c themec.h pgtheme.h


INCLUDES = -I$(top_srcdir)/include

YACC = @YACC@ -d

MKSYMTAB := perl mksymtab.pl

LDADD = @LEXLIB@

CONSTANTS := $(top_srcdir)/include/picogui/*.h

DISTCLEANFILES = symtab.c pgtheme.c pgtheme.h pglex.c

EXTRA_DIST = mksymtab.pl

symtab.c : $(CONSTANTS) mksymtab.pl
	$(MKSYMTAB) $(CONSTANTS) > symtab.c

