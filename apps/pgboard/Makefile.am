bin_PROGRAMS = pgboard pgboard_cmds
pgboard_SOURCES = pgboard.c kbfile.h kbfile.c ntohs_fr.h
pgboard_cmds_SOURCES = pgboard_cmds.c

EXTRA_DIST = kbcompile.pl

LDFLAGS += @PICOGUI_LDFLAGS@
LDADD = @PICOGUI_LIBS@

CFLAGS = @CFLAGS@ @PICOGUI_CFLAGS@

SUBDIRS = examples
