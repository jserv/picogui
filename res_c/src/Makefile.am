INCLUDES = -I$(top_srcdir)/include 

lib_LTLIBRARIES = libres_c.la

libres_c_la_SOURCES = \
	confparse.c \
	res_c.c \
	$(PLATFORM)

EXTRA_libres_c_la_SOURCES = \
	picogui_support.c

libres_c_la_LIBADD       = $(INTERFACES:%.c=%.lo)
libres_c_la_DEPENDENCIES = $(INTERFACES:%.c=%.lo)
