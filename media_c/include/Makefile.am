PGSERVER_DIR = @PGSERVER_DIR@

includedir = @includedir@/res_c

include_HEADERS = \
	confparse.h \
	res_c.h 

install-data-hook:
	if ! test x"$(PGSERVER_DIR)" = x$(prefix) ; then \
		echo "Copying res_c's include files ..." ; \
		cp $(PGSERVER_DIR)/include/res_c/*.h $(DESTDIR)$(includedir)/ || true; \
	fi
