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
	x11.c      \
	mgl2fb.c   \
	sdlgl_bitmap.c \
	sdlgl_camera.c \
	sdlgl_init.c \
	sdlgl_primitives.c \
	sdlgl_util.c \
	sdlgl_grop.c

noinst_HEADERS = \
	yuv.h

libvideo_la_LIBADD       = $(DRIVER:%.c=%.lo)
libvideo_la_DEPENDENCIES = $(DRIVER:%.c=%.lo)


