INCLUDES = -I$(top_builddir)/include -I$(top_srcdir)/include

noinst_LIBRARIES = libvidbase.a

libvidbase_a_SOURCES = 	\
	dvbl_bitmap.c   \
	dvbl_color.c	\
	dvbl_effects.c	\
	dvbl_init.c	\
	dvbl_lgop.c	\
	dvbl_sprite.c	\
	dvbl_font.c	\
	font_base.c	\
	$(VIDBASE)

EXTRA_libvidbase_a_SOURCES = \
	linear1.c	\
	linear2.c	\
	linear4.c	\
	linear8.c	\
	linear16.c      \
	linear24.c      \
	linear32.c      \
	slowvbl.c       \
	dvbl_dither.c   \
	visibone2.c     \
	dvbl_primitives.c \
	dvbl_primitives_aa.c \
	font_bdf.c	\
	font_freetype.c

libvidbase_a_LIBADD       = $(VIDBASE:%.c=%.o)
libvidbase_a_DEPENDENCIES = $(VIDBASE:%.c=%.o)


