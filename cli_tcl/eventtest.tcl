#!/usr/bin/tclsh
source picogui.tcl
pgConnect localhost 0

pgRegisterApp "Greetings" $pg_app(normal)
set b [pgNewWidget $pg_widget(canvas)]
pgSetSide $b all
pgBind $b any {parray "event"}
#pgBind $b $pg_we(activate) update
pgUpdate
pgEventLoop
