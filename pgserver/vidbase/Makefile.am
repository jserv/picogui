INCLUDES = -I$(top_builddir)/include -I$(top_srcdir)/include

noinst_LIBRARIES = libvidbase.a

libvidbase_a_SOURCES = 	\
	defaultvbl.c	\
	$(VIDBASE)

EXTRA_libvidbase_a_SOURCES = \
	linear1.c	\
	linear2.c	\
	linear4.c	\
	linear8.c	\
	linear16.c

libvidbase_a_LIBADD       = $(VIDBASE:%.c=%.o)
libvidbase_a_DEPENDENCIES = $(VIDBASE:%.c=%.o)


