kbddir = ${prefix}/share/pgboard
kbd_DATA = us_fitaly_160.kb us_qwerty_240.kb us_qwerty_scalable.kb ch_qwertz_scalable.kb \
	   us_qwerty_scalable_console8.kb us_qwerty_scalable_helvetica8.kb \
	   us_qwerty_scalable_clean6.kb \
	   us_qwerty_scalable_color.kb us_dvorak_scalable.kb us_dvorak_scalable_color.kb

EXTRA_DIST = us_fitaly_160.kbs us_qwerty_scalable.kbs \
	     ch_qwertz_scalable.kbs img/fitaly_caps.pbm \
	     img/fitaly_control.pbm img/fitaly_default.pbm \
	     img/fitaly_numbers.pbm img/fitaly_shift.pbm \
	     img/fitaly_symbols.pbm us_qwerty_scalable_console8.kbs \
	     us_querty_scalable_clean6.kbs \
	     us_qwerty_scalable_color.kbs \
	     us_dvorak_scalable.kbs \
	     us_dvorak_scalable_color.kbs \
	     us_qwerty_scalable_helvetica8.kbs \
             us_qwerty_240.kbs \
             img/us_qwerty_240_alpha.pbm \
             img/us_qwerty_240_shift.pbm \
             img/us_qwerty_240_cap.pbm \
             img/us_qwerty_240_ctrl.pbm \
             img/us_qwerty_240_num.pbm \
             img/us_qwerty_240_sym.pbm \
             img/us_qwerty_240_int.pbm \
             img/us_qwerty_240_shiftint.pbm

CLEANFILES = $(top_srcdir)/working/pgboard/examples/us_fitaly_160.kb \
	     $(top_srcdir)/working/pgboard/examples/us_qwerty_240.kb \
	     $(top_srcdir)/working/pgboard/examples/us_qwerty_scalable.kb \
	     $(top_srcdir)/working/pgboard/examples/us_qwerty_scalable_color.kb \
	     $(top_srcdir)/working/pgboard/examples/us_qwerty_scalable_clean6.kb \
	     $(top_srcdir)/working/pgboard/examples/us_dvorak_scalable.kb \
	     $(top_srcdir)/working/pgboard/examples/us_dvorak_scalable_color.kb \
	     $(top_srcdir)/working/pgboard/examples/ch_qwertz_scalable.kb \
	     $(top_srcdir)/working/pgboard/examples/us_qwerty_scalable_console8.kb \
	     $(top_srcdir)/working/pgboard/examples/us_qwerty_scalable_helvetica8.kb

KBD_COMPILER = ../kbcompile.pl -pg @PICOGUI_LOCATION@

us_fitaly_160.kb: $(top_srcdir)/working/pgboard/examples/us_fitaly_160.kbs \
                  $(top_srcdir)/working/pgboard/examples/img/fitaly_default.pbm \
                  $(top_srcdir)/working/pgboard/examples/img/fitaly_shift.pbm \
                  $(top_srcdir)/working/pgboard/examples/img/fitaly_caps.pbm \
                  $(top_srcdir)/working/pgboard/examples/img/fitaly_symbols.pbm \
                  $(top_srcdir)/working/pgboard/examples/img/fitaly_control.pbm \
                  $(top_srcdir)/working/pgboard/examples/img/fitaly_numbers.pbm
	cd $(top_srcdir)/working/pgboard/examples && $(KBD_COMPILER) < us_fitaly_160.kbs > us_fitaly_160.kb

us_qwerty_240.kb: $(top_srcdir)/working/pgboard/examples/us_qwerty_240.kbs \
                  $(top_srcdir)/working/pgboard/examples/img/us_qwerty_240_alpha.pbm \
                  $(top_srcdir)/working/pgboard/examples/img/us_qwerty_240_shift.pbm \
                  $(top_srcdir)/working/pgboard/examples/img/us_qwerty_240_cap.pbm \
                  $(top_srcdir)/working/pgboard/examples/img/us_qwerty_240_ctrl.pbm \
                  $(top_srcdir)/working/pgboard/examples/img/us_qwerty_240_num.pbm \
                  $(top_srcdir)/working/pgboard/examples/img/us_qwerty_240_sym.pbm \
                  $(top_srcdir)/working/pgboard/examples/img/us_qwerty_240_int.pbm \
                  $(top_srcdir)/working/pgboard/examples/img/us_qwerty_240_shiftint.pbm
	cd $(top_srcdir)/working/pgboard/examples && $(KBD_COMPILER) < us_qwerty_240.kbs > us_qwerty_240.kb

us_qwerty_scalable.kb: $(top_srcdir)/working/pgboard/examples/us_qwerty_scalable.kbs
	cd $(top_srcdir)/working/pgboard/examples && $(KBD_COMPILER) < us_qwerty_scalable.kbs > us_qwerty_scalable.kb

us_dvorak_scalable.kb: $(top_srcdir)/working/pgboard/examples/us_dvorak_scalable.kbs
	cd $(top_srcdir)/working/pgboard/examples && $(KBD_COMPILER) < us_dvorak_scalable.kbs > us_dvorak_scalable.kb

us_qwerty_scalable_console8.kb: $(top_srcdir)/working/pgboard/examples/us_qwerty_scalable_console8.kbs
	cd $(top_srcdir)/working/pgboard/examples && $(KBD_COMPILER) < us_qwerty_scalable_console8.kbs > us_qwerty_scalable_console8.kb

us_qwerty_scalable_clean6.kb: $(top_srcdir)/working/pgboard/examples/us_qwerty_scalable_clean6.kbs
	cd $(top_srcdir)/working/pgboard/examples && $(KBD_COMPILER) < us_qwerty_scalable_clean6.kbs > us_qwerty_scalable_clean6.kb

us_qwerty_scalable_helvetica8.kb: $(top_srcdir)/working/pgboard/examples/us_qwerty_scalable_helvetica8.kbs
	cd $(top_srcdir)/working/pgboard/examples && $(KBD_COMPILER) < us_qwerty_scalable_helvetica8.kbs > us_qwerty_scalable_helvetica8.kb

ch_qwertz_scalable.kb: $(top_srcdir)/working/pgboard/examples/ch_qwertz_scalable.kbs
	cd $(top_srcdir)/working/pgboard/examples && $(KBD_COMPILER) < ch_qwertz_scalable.kbs > ch_qwertz_scalable.kb

us_qwerty_scalable_color.kb: $(top_srcdir)/working/pgboard/examples/us_qwerty_scalable_color.kbs
	cd $(top_srcdir)/working/pgboard/examples && $(KBD_COMPILER) < us_qwerty_scalable_color.kbs > us_qwerty_scalable_color.kb

us_dvorak_scalable_color.kb: $(top_srcdir)/working/pgboard/examples/us_dvorak_scalable_color.kbs
	cd $(top_srcdir)/working/pgboard/examples && $(KBD_COMPILER) < us_dvorak_scalable_color.kbs > us_dvorak_scalable_color.kb
