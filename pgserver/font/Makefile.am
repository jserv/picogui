INCLUDES = -I$(top_builddir)/include -I$(top_srcdir)/include

noinst_LIBRARIES = libfont.a

BUILT_SOURCES = fonts.c

libfont_a_SOURCES = fonts.c

FDF_FILES = \
	clean12.fdf \
	clean6.fdf \
	console8.fdf \
	courier12b.fdf \
	courier12.fdf \
	helvetica10.fdf \
	helvetica12b.fdf \
	helvetica12.fdf \
	helvetica14b.fdf \
	helvetica14.fdf \
	helvetica18b.fdf \
	helvetica18.fdf \
	helvetica24b.fdf \
	helvetica24.fdf \
	helvetica8.fdf \
	lucida10.fdf \
	times10b.fdf \
	times10.fdf \
	utopia25.fdf

FI_FILES = \
	clean12.fi \
	clean6.fi \
	console8.fi \
	courier12.fi \
	helvetica10.fi \
	helvetica12.fi \
	helvetica14.fi \
	helvetica18.fi \
	helvetica24.fi \
	helvetica8.fi \
	lucida10.fi \
	times10.fi \
	utopia25.fi

EXTRA_DIST = $(FDF_FILES) $(FI_FILES)


libfont_a_DEPENDENCIES = \
	$(srcdir)/*.fdf \
	$(srcdir)/*.fi  \
	$(top_srcdir)/script/fontdef.pl \
	@CONFIG_FILE@


fonts.c: $(CONFIG_FILE)
	$(PERL) $(top_srcdir)/script/fontdef.pl \
		$(CONFIG_FILE) $(srcdir) > fonts.c
