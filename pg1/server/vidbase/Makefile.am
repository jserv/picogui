INCLUDES = -I$(top_builddir)/include -I$(top_srcdir)/include

noinst_LTLIBRARIES = libvidbase.la

libvidbase_la_SOURCES = \
	dvbl_bitmap.c   \
	dvbl_color.c	\
	dvbl_effects.c	\
	dvbl_init.c	\
	dvbl_lgop.c	\
	dvbl_sprite.c	\
	dvbl_font.c	\
	font_base.c	\
	$(VIDBASE)

EXTRA_libvidbase_la_SOURCES = \
	linear1.c	\
	linear2.c	\
	linear4.c	\
	linear8.c	\
	linear16.c      \
	linear24.c      \
	linear32.c      \
	yuv16_422_planar.c \
	slowvbl.c       \
	dvbl_dither.c   \
	visibone2.c     \
	dvbl_primitives.c \
	dvbl_primitives_aa.c \
	font_bdf.c	\
	font_freetype.c	\
	font_textmode.c	\
	font_xft.c	\
	font_ftgl.c	\
	gl_bitmap.c	\
	gl_camera.c	\
	gl_init.c	\
	gl_primitives.c	\
	gl_util.c	\
	gl_grop.c

noinst_HEADERS = \
	yuv.h

libvidbase_la_LIBADD       = $(VIDBASE:%.c=%.lo)
libvidbase_la_DEPENDENCIES = $(VIDBASE:%.c=%.lo)
