DEPS = picoconstants.in widgets.in picogui.in

picogui.tcl: $(DEPS)
	cat $(DEPS) > $@

apps/parseheader.tcl: tclheader apps/parseheader.in
	cat $< apps/parseheader.in >$@
	chmod +x $@

tclheader:
	echo '#!'`which tclsh` >tclheader

widgets.in: ../pgserver/include/picogui/constants.h apps/parseheader.tcl
	apps/parseheader.tcl $< $@ PG_WIDGET_ pg_widget

clean:
	rm -f apps/*.tcl picogui.tcl tclheader widgets.in
