bin_PROGRAMS = picomail

picomail_SOURCES = configfile.c imap.c picomail.c tcpclient.c

EXTRA_DIST = picomail.conf

LDFLAGS += @PICOGUI_LDFLAGS@
LDADD = @PICOGUI_LIBS@

CFLAGS = @CFLAGS@ @PICOGUI_CFLAGS@
