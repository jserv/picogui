#!/usr/bin/tclsh
source picogui.tcl
pgui connect localhost 0

set p [pgui register -title "Button Test" -type toolbar]

set l [pgwidget create button -text "Hello World" -side all -font \
	[pgui create font  -style $pg_fstyle(bold) -size 24]]

pgwidget attach  $l inside $p
pgui update
pgwidget bind $l activate {puts "hello"}
pgwidget bind any close {puts "goodby"; exit}
pgEventLoop
