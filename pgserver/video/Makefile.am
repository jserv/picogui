INCLUDES = -I$(top_builddir)/include -I$(top_srcdir)/include $(DRIVER_CFLAGS)

noinst_LIBRARIES = libvideo.a

libvideo_a_SOURCES = \
    $(DRIVER)

EXTRA_libvideo_a_SOURCES = \
	ez328.c   \
	fbdev.c   \
	ncurses.c \
	null.c 	  \
	sdlfb.c   \
	svgafb.c  \
	serial40x4.c

libvideo_a_LIBADD       = $(DRIVER:%.c=%.o)
libvideo_a_DEPENDENCIES = $(DRIVER:%.c=%.o)


