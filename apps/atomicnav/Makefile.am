bin_PROGRAMS = atomicnav

atomicnav_SOURCES = main.c \
                    browserwin.c \
                    url.c \
                    protocol.c \
                    p_http.c \
                    p_file.c

LDFLAGS += @PICOGUI_LDFLAGS@
LDADD = @PICOGUI_LIBS@

CFLAGS = @CFLAGS@ @PICOGUI_CFLAGS@
