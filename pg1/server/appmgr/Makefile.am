INCLUDES = -I$(top_builddir)/include -I$(top_srcdir)/include

noinst_LTLIBRARIES = libappmgr.la

libappmgr_la_SOURCES = appmgr.c

EXTRA_libappmgr_la_SOURCES = \
    panel.c \
    null.c \
    managed_rootless.c

libappmgr_la_LIBADD       = $(APPMGR:%.c=%.lo)
libappmgr_la_DEPENDENCIES = $(APPMGR:%.c=%.lo)


