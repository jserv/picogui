bin_PROGRAMS = pgboard pgboard_cmds

pgboard_SOURCES = pgboard.h pgboard.c kbfile.h kbfile.c ntohs_fr.h
pgboard_cmds_SOURCES = pgboard_cmds.c

lib_LTLIBRARIES = libpgboard.la
libpgboard_la_SOURCES = pgboard_api.h pgboard_api.c

includedir = @includedir@/picogui
include_HEADERS = pgboard.h pgboard_api.h

EXTRA_DIST = kbcompile.pl

LDFLAGS += @PICOGUI_LDFLAGS@ @PM_LDFLAGS@
LDADD = @PICOGUI_LIBS@ @PM_LIBS@

CFLAGS = @CFLAGS@ @PICOGUI_CFLAGS@ @PM_CFLAGS@

SUBDIRS = examples
