bin_PROGRAMS = themec

themec_SOURCES = pgtheme.y pglex.l main.c backend.c symtab.c themec.h pgtheme.h


INCLUDES = -I$(top_srcdir)/include

YACC = @YACC@ -d

MKSYMTAB := perl $(srcdir)/mksymtab.pl

LDADD = @LEXLIB@

# Standard PicoGUI headers that would be of interest to themes

CONSTANTS := $(top_srcdir)/include/picogui/constants.h \
             $(top_srcdir)/include/picogui/pgkeys.h    \
             $(top_srcdir)/include/picogui/canvas.h

# Some extras for OpenGL support, but only if available

GLHEADER := /usr/include/GL/gl.h
if OPENGL
  CONSTANTS := $(CONSTANTS) \
	       $(top_srcdir)/include/picogui/opengl.h \
	       $(GLHEADER)
endif

DISTCLEANFILES = symtab.c pgtheme.c pgtheme.h pglex.c

EXTRA_DIST = $(srcdir)/mksymtab.pl

symtab.c : $(CONSTANTS) $(srcdir)/mksymtab.pl
	$(MKSYMTAB) $(CONSTANTS) > symtab.c

