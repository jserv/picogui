#!/usr/bin/tclsh
source picogui.tcl

set NUMFRAMES 13
set imagebase "../apps/bouncyball/data/ball%02d.jpeg"

#picogui connect -server localhost -display 0
picogui connect
if { $connection == 0 } {
	puts "unable to connect to display"
	exit 1
}

set context [picogui entercontext]

set imgnr 0
while {$imgnr<$NUMFRAMES} {
	set img($imgnr) [picogui create bitmap -name [format $imagebase $imgnr]]
	incr imgnr
}
set dlg [picowidget create dialogbox -text Boing!]
set ok [picowidget create button -text Ok -side bottom]
set bmp [picowidget create label -side all]

picowidget attach $ok inside $dlg
picowidget attach $bmp after $ok

set i 0
set d 1
while { 1 } {
	picowidget set $bmp -bitmap $img($i)
	picogui update
	if { [picoevent check] >0 } {
		array set event [picoevent get]
		if { $event(from) == $ok } {
			break
		}
		parray event
	}
	if { $i == [expr $NUMFRAMES - 1] } {
		set d -1
	}
	if { $i == 0 } {
		set d 1
	}
	incr i $d
}
picogui leavecontext -id $context
