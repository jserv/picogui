#!/usr/bin/tclsh
source picogui.tcl
pgui connect localhost 0

pgRegisterApp "Button Test" $pg_app(normal)
set l [pgNewButton "Hello World"]
pgSetFont $l [pgNewFont "" $pg_fstyle(bold) 24]
pgSetSide $l all
pgUpdate
pgBind $l $pg_we(activate) {puts "hello"}
pgBind any $pg_we(close) {puts "goodby"; exit}
pgEventLoop
