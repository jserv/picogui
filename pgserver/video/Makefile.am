INCLUDES = -I$(top_builddir)/include -I$(top_srcdir)/include $(DRIVER_CFLAGS)

noinst_LIBRARIES = libvideo.a

libvideo_a_SOURCES = \
    $(DRIVER)

EXTRA_libvideo_a_SOURCES = \
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
	sdlgl_font.c \
	sdlgl_init.c \
	sdlgl_primitives.c \
	sdlgl_util.c \
	sdlgl_grop.c

noinst_HEADERS = \
	yuv.h

libvideo_a_LIBADD       = $(DRIVER:%.c=%.o)
libvideo_a_DEPENDENCIES = $(DRIVER:%.c=%.o)


