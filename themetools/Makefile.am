SUBDIRS = thdump themec 

dist-hook:
	$(mkinstalldirs) $(distdir)/examples/data
	$(mkinstalldirs) $(distdir)/include/picogui
	cp -ra $(srcdir)/examples/*.ths $(distdir)/examples
	cp -ra $(srcdir)/examples/data/*.pnm $(distdir)/examples/data
	cp -ra $(srcdir)/examples/data/README $(distdir)/examples/data
	cp -ra $(srcdir)/include/picogui/*.h $(distdir)/include/picogui