#!/usr/bin/tclsh
source picogui.tcl
pgui connect localhost 0

set p [pgRegisterApp "Event Test" $pg_app(normal)]
set b [pgCreateWidget canvas]
pgAttach $b inside $p
pgSetSide $b all
pgSetWidget $b $pg_wp(triggermask) $pg_trigger(move)
pgBind $b any {parray "event"}
pgui update
pgEventLoop
