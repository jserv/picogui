#!/usr/bin/tclsh
source picogui.tcl

set NUMFRAMES 13
set imagebase "../apps/bouncyball/data/ball%02d.jpeg"

#pgui connect -server localhost -display 0
pgui connect
if { $connection == 0 } {
	puts "unable to connect to display"
	exit 1
}

set context [pgui entercontext]

set imgnr 0
while {$imgnr<$NUMFRAMES} {
	set img($imgnr) [pgui createbitmap -name [format $imagebase $imgnr]]
	incr imgnr
}
set dlg [pgwidget create dialogbox]
set ok [pgwidget create button]
set bmp [pgwidget create label]

pgwidget set $dlg -text "Boing!"
pgwidget set $ok -side bottom -text "Ok"
pgwidget set $bmp -side all

pgwidget attach $ok inside $dlg
pgwidget attach $bmp after $ok

set i 0
set d 1
while { 1 } {
	pgwidget set $bmp -bitmap $img($i)
#	pgSetBitmap $bmp $img($i)
	pgui update
	if { [pgui checkevent] >0 } {
		array set event [pgui waitevent]
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
pgui leavecontext -id $context
