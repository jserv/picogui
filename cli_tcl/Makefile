DEPS = widgets.in requests.in pg_wp.in pgth_o.in pg_vid.in pg_we.in \
	pg_fstyle.in pg_trigger.in\
	picoconstants.in picogui.in

picogui.tcl: $(DEPS)
	cat $(DEPS) > $@

apps/parseheader.tcl: tclheader apps/parseheader.in
	cat $< apps/parseheader.in >$@
	chmod +x $@

tclheader:
	echo '#!'`which tclsh` >tclheader

widgets.in: ../pgserver/include/picogui/constants.h apps/parseheader.tcl
	apps/parseheader.tcl $< $@ PG_WIDGET_ pg_widget

pgth_o.in: ../pgserver/include/picogui/constants.h apps/parseheader.tcl
	apps/parseheader.tcl $< $@ PGTH_O_ pgth_o

pg_wp.in: ../pgserver/include/picogui/constants.h apps/parseheader.tcl
	apps/parseheader.tcl $< $@ PG_WP_ pg_wp

pg_we.in: ../pgserver/include/picogui/constants.h apps/parseheader.tcl
	apps/parseheader.tcl $< $@ PG_WE_ pg_we

pg_vid.in: ../pgserver/include/picogui/constants.h apps/parseheader.tcl
	apps/parseheader.tcl $< $@ PG_VID_ pg_vid

pg_fstyle.in: ../pgserver/include/picogui/constants.h apps/parseheader.tcl
	apps/parseheader.tcl $< $@ PG_FSTYLE_ pg_fstyle

requests.in: ../pgserver/include/picogui/network.h apps/parseheader.tcl
	apps/parseheader.tcl $< $@ PGREQ_ pg_request

trigger.in: ../pgserver/include/picogui/network.h apps/parseheader.tcl
	apps/parseheader.tcl $< $@ PG_TRIGGER_ pg_trigger

clean:
	rm -f apps/*.tcl picogui.tcl tclheader widgets.in requests.in \
		pg_wp.in pgth_o.in pg_we.in pg_vid.in pg_fstyle.in \
		pg_trigger.in
