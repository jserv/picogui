if ENABLE_BEE

SUBDIRS = pgboard pterm fieldtest vrcalc omnibar pgtuxphone gridgame \
	  battleship galaxy imgview dialogdemo scribble canvastst bouncyball \
	  atomicnav

else

SUBDIRS = pgboard pterm fieldtest vrcalc omnibar pgtuxphone blackout gridgame \
	  battleship galaxy imgview dialogdemo scribble canvastst bouncyball \
	  atomicnav

endif

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
