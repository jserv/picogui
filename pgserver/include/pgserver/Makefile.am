BUILT_SOURCES = autoconf.h

includedir = @includedir@/pgserver

include_HEADERS = \
	appmgr.h 	\
	autoconf.h 	\
	common.h 	\
	divtree.h 	\
	g_error.h 	\
	g_malloc.h 	\
	handle.h 	\
	inlstring.h 	\
	input.h 	\
	pgnet.h 	\
	render.h 	\
	svrtheme.h 	\
	timer.h 	\
	video.h 	\
	widget.h	\
	configfile.h	\
	hotspot.h	\
	textbox.h	\
	gl.h		\
	fbkeys.h	\
	svrwt.h		\
	pgstring.h	\
	paragraph.h     \
	terminal.h      \
	textedit.h	\
	llist.h		\
	font.h		\
	types.h		\
	font_bdf.h	\
	init.h		\
	os.h		\
	os_posix.h	\
	x11.h           \
	requests.h	\
       	debug.h

CONFIG_CLEAN_FILES = autoconf.h

autoconf.h: $(CONFIG_FILE) $(top_builddir)/config.status \
	$(top_srcdir)/config.in $(top_srcdir)/configscript/Configure
	sed "s/.*//g" $(top_srcdir)/config.in | \
		sh $(top_srcdir)/configscript/Configure \
		$(top_srcdir)/config.in @CONFIG_FILE@ > /dev/null
	rm -f .config autoconf.h && mv .config.h autoconf.h
