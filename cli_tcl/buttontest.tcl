#!/usr/bin/tclsh
source picogui.tcl
pgui connect localhost 0

set p [pgRegisterApp "Button Test" $pg_app(normal)]
set l [pgCreateWidget button]
pgAttach $l inside $p
pgSetText $l "Hello World"
pgSetFont $l [pgNewFont "" $pg_fstyle(bold) 24]
pgSetSide $l all
pgui update
pgBind $l $pg_we(activate) {puts "hello"}
pgBind any $pg_we(close) {puts "goodby"; exit}
pgEventLoop
