INCLUDES = -I$(top_builddir)/include -I$(top_srcdir)/include

noinst_LIBRARIES = libgcore.a

libgcore_a_SOURCES = \
    grop.c \
    render.c \
    g_malloc.c \
    div.c \
    font.c \
    handle.c \
    g_error.c \
    timer.c \
    errortext.c \
    video.c \
    input.c \
    driverinfo.c

EXTRA_DIST = inputdrivers.inc videodrivers.inc

libgcore_a_DEPENDENCIES = inputdrivers.inc videodrivers.inc


