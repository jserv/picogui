#!/usr/bin/tclsh
source picogui.tcl
pgConnect localhost 0

pgRegisterApp "Greetings" $pg_app(normal)
set l [pgNewLabel "Hello World"]
pgSetFont $l [pgNewFont "" $pg_fstyle(bold) 24]
pgSetSide $l all
pgUpdate
pgEventLoop
