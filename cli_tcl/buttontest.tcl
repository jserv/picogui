#!/usr/bin/tclsh
source picogui.tcl
pgui connect localhost 0

set p [pgRegisterApp "Button Test" $pg_app(normal)]
set l [pgwidget create button]
pgwidget attach  $l inside $p
pgwidget set $l -text "Hello World" -side all
pgSetFont $l [pgNewFont "" $pg_fstyle(bold) 24]
pgui update
pgBind $l $pg_we(activate) {puts "hello"}
pgBind any $pg_we(close) {puts "goodby"; exit}
pgEventLoop
