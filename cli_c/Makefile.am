SUBDIRS = src include


# ruiles for compiling in the Bee environment
if ENABLE_BEE

dist-hook: @PACKAGE@.spec
	for file in PicoGUIClient.mdl ; do \
	  cp -a $(srcdir)/$${file} $(distdir)/$${file} ; done
	cp -a @PACKAGE@.spec $(distdir)/@PACKAGE@.spec

@RPM_MAKE_FRAG@

else

dist-hook:
	@echo "nothing to add"

endif
