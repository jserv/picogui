SUBDIRS = $(VIDSUBDIRS)

INCLUDES = -I$(top_builddir)/include -I$(top_srcdir)/include $(DRIVER_CFLAGS)

noinst_LTLIBRARIES = libvideo.la

libvideo_la_SOURCES = \
	video_drivers.c \
	$(DRIVER)

EXTRA_libvideo_la_SOURCES = \
	ez328.c    \
	ez328vga.c \
	fbdev.c    \
	ncurses.c  \
	null.c 	   \
	nullfb.c   \
	sdlfb.c    \
	svgafb.c   \
	serial40x4.c \
	scrshot.c  \
	s1d13806.c \
	x11_init.c \
	x11_window.c \
	x11_bitmap.c \
	x11_primitives.c \
	mgl2fb.c \
	sdlgl.c \
	glcontext.c \
	vncserver.c

libvideo_la_LIBADD       = $(DRIVER:%.c=%.lo)
libvideo_la_DEPENDENCIES = $(DRIVER:%.c=%.lo)


