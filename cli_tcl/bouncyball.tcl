#!/usr/bin/tclsh
source picogui.tcl
source stddlg.tcl

set NUMFRAMES 13
set imagebase "../apps/bouncyball/data/ball%02d.jpeg"
pgConnect localhost 0
if { $connection == 0 } {
	puts "unable to connect to display"
}

pgEnterContext

set imgnr 0
while {$imgnr<$NUMFRAMES} {
	set img($imgnr) [pgLoadBitmap [format $imagebase $imgnr]]
	incr imgnr
}

set dlg [pgDialog "Boing!"]
set ok [pgNewButton "Ok"]
pgSetSide $ok bottom

set bmp [pgNewBitmap $img(0)]
pgSetSide $bmp all

set i 0
set d 1
while { 1 } {
	pgSetBitmap $bmp $img($i)
	pgUpdate
	if { [pgCheckEvent] >0 } {
		array set event [pgWaitEvent]
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
pgLeaveContext