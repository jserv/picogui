#!/usr/bin/tclsh
source picogui.tcl
pgui connect localhost 0

set p [pgRegisterApp "Greetings" $pg_app(normal)]
set l [pgCreateWidget label]
pgSetText $l "Hello World!"
pgAttach $l inside $p
pgSetFont $l [pgNewFont "" $pg_fstyle(bold) 24]
pgSetSide $l all
pgui update
pgEventLoop
