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

pgui entercontext

set imgnr 0
while {$imgnr<$NUMFRAMES} {
	set img($imgnr) [pgui createbitmap -name [format $imagebase $imgnr]]
	incr imgnr
}
set dlg [pgDialog "Boing!"]
set ok [pgCreateWidget button]
set bmp [pgNewBitmap $img(0)]

pgSetText $ok "Ok"
pgSetSide $ok bottom
pgSetSide $bmp all
pgSetWidget $bmp $pg_wp(transparent) 0
pgAttach $ok inside $dlg
pgAttach $bmp after $ok

set i 0
set d 1
while { 1 } {
	pgSetBitmap $bmp $img($i)
	pgui update
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
