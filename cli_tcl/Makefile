DEPS = widgets.in requests.in pg_wp.in pgth_o.in pg_vid.in pg_we.in \
	pg_fstyle.in trigger.in exev.in response.in eventcoding.in \
	side.in flagmode.in derive.in app.in \
	picogui.in

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

response.in: ../pgserver/include/picogui/network.h apps/parseheader.tcl
	apps/parseheader.tcl $< $@ PG_RESPONSE_ pg_response

trigger.in: ../pgserver/include/picogui/constants.h apps/parseheader.tcl
	apps/parseheader.tcl $< $@ PG_TRIGGER pg_trigger

exev.in: ../pgserver/include/picogui/constants.h apps/parseheader.tcl
	apps/parseheader.tcl $< $@ PG_EXEV_ pg_exev

eventcoding.in: ../pgserver/include/picogui/constants.h apps/parseheader.tcl
	apps/parseheader.tcl $< $@ PG_EVENTCODING pg_eventcoding

side.in: ../pgserver/include/picogui/constants.h apps/parseheader.tcl
	apps/parseheader.tcl $< $@ PG_S_ pg_s

flagmode.in: ../pgserver/include/picogui/constants.h apps/parseheader.tcl
	apps/parseheader.tcl $< $@ PG_FM_ pg_fm

derive.in: ../pgserver/include/picogui/constants.h apps/parseheader.tcl
	apps/parseheader.tcl $< $@ PG_DERIVE_ pg_derive

app.in: ../pgserver/include/picogui/constants.h apps/parseheader.tcl
	apps/parseheader.tcl $< $@ PG_APP_ pg_app

clean:
	rm -f apps/*.tcl picogui.tcl tclheader widgets.in requests.in \
		pg_wp.in pgth_o.in pg_we.in pg_vid.in pg_fstyle.in \
		trigger.in derive.in exev.in flagmode.in side.in app.in \
		response.in eventcoding.in
