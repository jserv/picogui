#!/usr/bin/tclsh
source picogui.tcl
picogui connect

set p [picogui register -title "Button Test" -type normal]

set l [picowidget create button -text "Hello World" -side all -font \
	[picogui create font  -style $pg_fstyle(bold) -size 24]]

picowidget attach  $l inside $p
picogui update
picoevent bind $l activate {puts "hello"}
picoevent bind any close {puts "goodby"; exit}
picoevent loop
