#!/usr/bin/tclsh
source picogui.tcl
pgConnect localhost 0

pgRegisterApp "Button Test" $pg_app(normal)
set l [pgNewButton "Hello World"]
pgSetFont $l [pgNewFont "" $pg_fstyle(bold) 24]
pgSetSide $l all
pgUpdate
puts [pgGetVideoMode]
while {1} {
	array set event [pgWaitEvent]
	parray event
}
