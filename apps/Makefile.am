if ENABLE_BEE

SUBDIRS = pgboard pterm fieldtest omnibar pgtuxphone atomicnav \
	  battleship galaxy imgview dialogdemo scribble canvastst hello pgmon \
	  msgdemo shm

else

SUBDIRS = pgboard pterm fieldtest vrcalc omnibar pgtuxphone blackout gridgame \
	  battleship galaxy imgview dialogdemo scribble canvastst bouncyball \
	  atomicnav tpcal pgl hello picomail pgmon wclock pqw canvastiles \
	  connectfour textedit msgdemo

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
