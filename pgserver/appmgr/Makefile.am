INCLUDES = -I$(top_builddir)/include -I$(top_srcdir)/include

noinst_LIBRARIES = libappmgr.a

libappmgr_a_SOURCES = appmgr.c

EXTRA_libappmgr_a_SOURCES = \
    panel.c \
    null.c \
    managed_rootless.c

libappmgr_a_LIBADD       = $(APPMGR:%.c=%.o)
libappmgr_a_DEPENDENCIES = $(APPMGR:%.c=%.o)


