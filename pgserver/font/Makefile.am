INCLUDES = -I$(top_builddir)/include -I$(top_srcdir)/include

noinst_LIBRARIES = libfont.a

BUILT_SOURCES = fonts.c

libfont_a_SOURCES = fonts.c

FDF_FILES = \
	clean12.bdf \
	clean6.bdf \
	console8.bdf \
	courier12b.bdf \
	courier12.bdf \
	helvetica10.bdf \
	helvetica12b.bdf \
	helvetica12.bdf \
	helvetica14b.bdf \
	helvetica14.bdf \
	helvetica18b.bdf \
	helvetica18.bdf \
	helvetica24b.bdf \
	helvetica24.bdf \
	helvetica8.bdf \
	lucida10.bdf \
	times10b.bdf \
	times10.bdf \
	utopia25.bdf \
	helvr08u.bdf \
	helvo14.bdf \
	helvb14.bdf \
   atari8.bdf

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
	utopia25.fi \
	helvb08u.fi \
	helvo14.fi \
	helvb14.fi \
   atari8.fi

EXTRA_DIST = $(FDF_FILES) $(FI_FILES)


fonts.c: $(CONFIG_FILE) $(top_srcdir)/script/fontdef.pl $(srcdir)/*.bdf $(srcdir)/*.fi
	$(PERL) $(top_srcdir)/script/fontdef.pl \
		$(CONFIG_FILE) $(srcdir) > fonts.c
