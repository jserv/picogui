#!/usr/bin/tclsh
source picogui.tcl
pgui connect localhost 0

set p [pgui register -title "Button Test" -type toolbar]

set l [pgwidget create button -text "Hello World" -side all -font \
	[pgNewFont "" $pg_fstyle(bold) 24]]

pgwidget attach  $l inside $p
pgui update
pgBind $l $pg_we(activate) {puts "hello"}
pgBind any $pg_we(close) {puts "goodby"; exit}
pgEventLoop
