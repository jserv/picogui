BUILT_SOURCES = autoconf.h

noinst_HEADERS = \
	appmgr.h \
	autoconf.h \
	common.h \
	divtree.h \
	font.h \
	g_error.h \
	g_malloc.h \
	handle.h \
	inlstring.h \
	input.h \
	mc68328digi.h \
	pgmain.h \
	pgnet.h \
	svrtheme.h \
	theme.h \
	timer.h \
	video.h \
	widget.h

CONFIG_CLEAN_FILES = autoconf.h

autoconf.h: $(CONFIG_FILE) $(top_builddir)/config.status \
	$(top_srcdir)/config.in $(top_srcdir)/configscript/Configure
	sed "s/.*//g" $(top_srcdir)/config.in | \
		sh $(top_srcdir)/configscript/Configure \
		$(top_srcdir)/config.in @CONFIG_FILE@ > /dev/null
	rm -f .config autoconf.h && mv .config.h autoconf.h
