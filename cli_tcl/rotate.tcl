#!/usr/bin/tclsh
source picogui.tcl

if {[catch {pgui connect -server localhost -display 0}] ==0} {
	exit 1
}

set rot [lindex $argv 0]
set fmode $pg_fm(on)
set flags $pg_vid(rotate90)
if { $argc == 0 } {
	set fmode $pg_fm(set)
	array set mode [pgGetVideoMode]
	if { [expr $mode(flags) & $pg_vid(rotate90)]} {
		set flags $pg_vid(rotate180)
	} elseif { [expr $mode(flags) & $pg_vid(rotate180)]} {
		set flags $pg_vid(rotate270)
	} elseif { [expr $mode(flags) & $pg_vid(rotate270)]} {
		set flags 0
	}
} elseif {$argc > 1} {
	puts "Too many arguments. usage $arg0 <0|90|180|270>"
} elseif {$argv==0} {
	set flags [expr $pg_vid(rotate90) | $pg_vid(rotate180)| \
		$pg_vid(rotate270)]
	set fmode $pg_fm(off)
} else {
	if {[catch {set flags $pg_vid(rotate$rot)}] != 0} {
		set flags 0		
	}
}
pgSetVideoMode 0 0 0 $fmode $flags
pgSetVideoMode 0 0 0 $pg_fm(off) [expr ~$flags]
