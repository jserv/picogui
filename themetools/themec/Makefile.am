bin_PROGRAMS = themec

themec_SOURCES = pgtheme.y pglex.l main.c backend.c symtab.c themec.h pgtheme.h


INCLUDES = -I$(top_srcdir)/include

YACC = @YACC@ -d

MKSYMTAB := perl $(srcdir)/mksymtab.pl

LDADD = @LEXLIB@

CONSTANTS := $(top_srcdir)/include/picogui/*.h $(wildcard /usr/include/GL/gl.h)

DISTCLEANFILES = symtab.c pgtheme.c pgtheme.h pglex.c

EXTRA_DIST = $(srcdir)/mksymtab.pl

symtab.c : $(CONSTANTS) $(srcdir)/mksymtab.pl
	$(MKSYMTAB) $(CONSTANTS) > symtab.c

