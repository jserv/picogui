PGSERVER_DIR = @PGSERVER_DIR@

includedir = @includedir@/picogui

include_HEADERS = \
	client_c.h \
	pgfx.h \
	stddialog.h \
	applet.h

install-data-hook:
	if ! test x"$(PGSERVER_DIR)" = x$(prefix) ; then \
		echo "Copying pgserver's include files ..." ; \
		cp $(PGSERVER_DIR)/include/picogui/*.h $(DESTDIR)$(includedir)/ || true; \
	fi
