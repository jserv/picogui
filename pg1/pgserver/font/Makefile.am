INCLUDES = -I$(top_builddir)/include -I$(top_srcdir)/include

noinst_LTLIBRARIES = libfont.la

BUILT_SOURCES = bdf_fonts.c

libfont_la_SOURCES =

EXTRA_libfont_la_SOURCES = bdf_fonts.c

FDF_FILES = \
        clean12.bdf \
        clean6.bdf \
        clean8.bdf \
        console8.bdf \
        courier12.bdf \
        courier12b.bdf \
        fixed13ja.bdf \
        helvetica10.bdf \
        helvetica12.bdf \
        helvetica12b.bdf \
        helvetica14.bdf \
        helvetica14b.bdf \
        helvetica14unicode.bdf \
        helvetica18.bdf \
        helvetica18b.bdf \
        helvetica24.bdf \
        helvetica24b.bdf \
        helvetica8.bdf \
        lucida10.bdf \
        times10.bdf \
        times10b.bdf \
        utopia25.bdf \
        youngatheart18.bdf

FI_FILES = \
        clean12.fi \
        clean6.fi \
        clean8.fi \
        console8.fi \
        courier12.fi \
        fixed13ja.fi \
        helvetica10.fi \
        helvetica12.fi \
        helvetica14.fi \
        helvetica14unicode.fi \
        helvetica18.fi \
        helvetica24.fi \
        helvetica8.fi \
        lucida10.fi \
        times10.fi \
        utopia25.fi \
        youngatheart18.fi

EXTRA_DIST = $(FDF_FILES) $(FI_FILES)

libfont_la_LIBADD       = $(FONT:%.c=%.lo)
libfont_la_DEPENDENCIES = $(FONT:%.c=%.lo)

bdf_fonts.c: $(CONFIG_FILE) $(top_srcdir)/script/fontdef.pl $(srcdir)/*.bdf $(srcdir)/*.fi
	$(PERL) $(top_srcdir)/script/fontdef.pl \
		$(CONFIG_FILE) $(srcdir) > bdf_fonts.c
