kbddir = ${prefix}/share/pgboard
kbd_DATA = us_fitaly_160.kb us_qwerty_240.kb us_qwerty_scalable.kb ch_qwertz_scalable.kb \
	   us_qwerty_scalable_console8.kb us_qwerty_scalable_helvetica8.kb

EXTRA_DIST = us_fitaly_160.kbs us_qwerty_scalable.kbs \
	     ch_qwertz_scalable.kbs img/fitaly_caps.pbm \
	     img/fitaly_control.pbm img/fitaly_default.pbm \
	     img/fitaly_numbers.pbm img/fitaly_shift.pbm \
	     img/fitaly_symbols.pbm us_qwerty_scalable_console8.kbs \
	     us_qwerty_scalable_helvetica8.kbs \
             us_qwerty_240.kbs \
             img/us_qwerty_240_alpha.pbm \
             img/us_qwerty_240_shift.pbm \
             img/us_qwerty_240_cap.pbm \
             img/us_qwerty_240_ctrl.pbm \
             img/us_qwerty_240_num.pbm \
             img/us_qwerty_240_sym.pbm

CLEANFILES = $(top_srcdir)/pgboard/examples/us_fitaly_160.kb \
	     $(top_srcdir)/pgboard/examples/us_qwerty_240.kb \
	     $(top_srcdir)/pgboard/examples/us_qwerty_scalable.kb \
	     $(top_srcdir)/pgboard/examples/ch_qwertz_scalable.kb \
	     $(top_srcdir)/pgboard/examples/us_qwerty_scalable_console8.kb \
	     $(top_srcdir)/pgboard/examples/us_qwerty_scalable_helvetica8.kb

KBD_COMPILER = ../kbcompile.pl -pg @PICOGUI_LOCATION@

us_fitaly_160.kb: $(top_srcdir)/pgboard/examples/us_fitaly_160.kbs
	cd $(top_srcdir)/pgboard/examples && $(KBD_COMPILER) < us_fitaly_160.kbs > us_fitaly_160.kb

us_qwerty_240.kb: $(top_srcdir)/pgboard/examples/us_qwerty_240.kbs
	cd $(top_srcdir)/pgboard/examples && $(KBD_COMPILER) < us_qwerty_240.kbs > us_qwerty_240.kb

us_qwerty_scalable.kb: $(top_srcdir)/pgboard/examples/us_qwerty_scalable.kbs
	cd $(top_srcdir)/pgboard/examples && $(KBD_COMPILER) < us_qwerty_scalable.kbs > us_qwerty_scalable.kb

us_qwerty_scalable_console8.kb: $(top_srcdir)/pgboard/examples/us_qwerty_scalable_console8.kbs
	cd $(top_srcdir)/pgboard/examples && $(KBD_COMPILER) < us_qwerty_scalable_console8.kbs > us_qwerty_scalable_console8.kb

us_qwerty_scalable_helvetica8.kb: $(top_srcdir)/pgboard/examples/us_qwerty_scalable_helvetica8.kbs
	cd $(top_srcdir)/pgboard/examples && $(KBD_COMPILER) < us_qwerty_scalable_helvetica8.kbs > us_qwerty_scalable_helvetica8.kb

ch_qwertz_scalable.kb: $(top_srcdir)/pgboard/examples/ch_qwertz_scalable.kbs
	cd $(top_srcdir)/pgboard/examples && $(KBD_COMPILER) < ch_qwertz_scalable.kbs > ch_qwertz_scalable.kb
