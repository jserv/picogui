bin_PROGRAMS = themec

themec_SOURCES = pgtheme.y pglex.l main.c backend.c symtab.c themec.h pgtheme.h

YACC = @YACC@ -d

MKSYMTAB := perl $(srcdir)/mksymtab.pl

LDADD = @LEXLIB@

# Standard PicoGUI headers that would be of interest to themes

CONSTANTS := $(PGSERVER_INCLUDES)/picogui/constants.h \
             $(PGSERVER_INCLUDES)/picogui/pgkeys.h

# Some extras for OpenGL support, but only if available

GLHEADER := /usr/include/GL/gl.h
if OPENGL
  CONSTANTS := $(CONSTANTS) \
	       $(PGSERVER_INCLUDES)/picogui/opengl.h \
	       $(GLHEADER)
endif

DISTCLEANFILES = symtab.c pgtheme.c pgtheme.h pglex.c

INCLUDES = $(PGSERVER_CFLAGS)

EXTRA_DIST = $(srcdir)/mksymtab.pl

symtab.c : $(CONSTANTS) $(srcdir)/mksymtab.pl
	$(MKSYMTAB) $(CONSTANTS) > symtab.c

