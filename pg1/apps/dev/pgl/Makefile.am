
SUBDIRS = pgl-toolbar pgl-launcher pgl-clock pgl-keyboard pgl-rotate

if ENABLE_BEE

dist-hook: @PACKAGE@.spec
	for file in PicoGUIApps.mdl ; do \
	  cp -a $(srcdir)/$${file} $(distdir)/$${file} ; done
	cp -a @PACKAGE@.spec $(distdir)/@PACKAGE@.spec

@RPM_MAKE_FRAG@

else

dist-hook:
	@echo "nothing to add"

endif
