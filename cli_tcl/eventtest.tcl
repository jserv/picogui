#!/usr/bin/tclsh
source picogui.tcl
pgConnect localhost 0

pgRegisterApp "Event Test" $pg_app(normal)
set b [pgNewWidget canvas]
pgSetSide $b all
pgSetWidget $b $pg_wp(triggermask) $pg_trigger(move)
pgBind $b any {parray "event"}
pgUpdate
pgEventLoop
