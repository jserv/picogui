source picogui.tcl
connect localhost 0
if { $connection == 0 } {
	puts "unable to connect to display"
}

set NUMFRAMES 13
pgEnterContext
set imgnr 0
while {$imgnr<$NUMFRAMES} {
	set img($imgnr) [pgNewBitmap [pgFromFile \
		[format "../apps/bouncyball/data/ball%02d.jpeg" $imgnr]]]
	incr imgnr
}

set dlg [pgDialog "Boing!"]
set ok [pgNewWidget $pg_derive(after) $pg_widget(BUTTON) $label]
pgSetWidget $ok $pg_s(bottom) $pg_wp(SIDE)
set id [pgNewString "OK"]
pgSetWidget $ok $id $pg_wp(TEXT)
set bmp [pgNewWidget $pg_derive(after) $pg_widget(BITMAP) $ok]
pgSetWidget $bmp $pg_s(all) $pg_wp(SIDE)
set i 0
set d 1
while { 1 } {
	pgSetWidget $bmp $img($i) $pg_wp(BITMAP)
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
