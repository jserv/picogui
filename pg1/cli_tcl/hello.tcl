#!/usr/bin/tclsh
source picogui.tcl
picogui connect localhost 0

set p [picogui register -title "Greetings" -type normal]
set l [picowidget create label -text "Hello World!" -side all -font \
	[picogui create font -style $pg_fstyle(bold) -size 24]
]
picowidget attach $l inside $p
picogui update
picoevent loop
