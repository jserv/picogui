package provide picogui 0.4

if { [catch {package require picoconstants}]!=0 } {
	source picoconstants.tcl
}
set binds(any) "any {parray event} $pg_we(close) exit"

set connection 0
set defaultparent 0
set defaultrship $pg_derive(inside)

proc pgGetResponse {} {
	global pg_response pg_request connection pg_eventcoding
	set data [read $connection 12]
	binary scan $data "S" type
	if { $type == $pg_response(error) } {
		binary scan $data "SSSSI" ret(type) ret(errt) ret(msglen) \
			ret(dummy) ret(id)
		set ret(msg) [read $connection $ret(msglen)]
	} elseif { $type == $pg_response(ret) } {
		binary scan $data "SSII" ret(type) ret(dummy) ret(id) ret(data)
	} elseif { $type == $pg_response(event) } {
		binary scan $data "SSII" type ret(event) ret(from) ret(param)
		if {[expr $ret(event) & $pg_eventcoding(pntr)]==$pg_eventcoding(pntr)} {
			puts "pointer parameters"
			set ret(x) [expr $ret(param) & 0xFFF]
			set ret(y) [expr ($ret(param) >>12) & 0xFFF]
			set ret(btn) [expr ($ret(param) >>24) & 0xF]
			set ret(chbtn) [expr ($ret(param) >>28) & 0xF]
		}
	} elseif { $type == $pg_response(data) } {
		binary scan $data "SSII" ret(type) ret(dummy) ret(id) ret(size)
		set ret(data) [read $connection $ret(size)]
	}
	return [array get ret]
}
proc pack_pgrequest { id size type {dummy 0} } {
	set req [binary format "IISS" $id $size $type $dummy]
	return $req
}
proc pgConnect {server display} {
	global connection
	set port [expr 30450 + $display] 
	if { [catch {set sock [socket $server $port]}] != 0 } {
		return
	}
	set data [read $sock 8]
	binary scan $data "ISS" magic protover dummy
	fconfigure $sock -translation binary
	set connection $sock
}
proc send_packet {packet} {
	global connection
	puts -nonewline $connection $packet
	flush $connection
}
proc pgSetVideoMode {xres yres bpp flagmode flags} {
	global pg_request
	send_packet [pack_pgrequest 1 12 $pg_request(setmode)]
	send_packet [binary format "SSSSI" $xres $yres $bpp $flagmode $flags]
	array set ret [pgGetResponse]
	return $ret(data)
}
proc pgGetVideoMode {} {
	global pg_request
	send_packet [pack_pgrequest 1 0 $pg_request(getmode)]
	array set data [pgGetResponse]
	binary scan $data(data) "ISSSSSS" res(flags) res(xres) res(yres) \
		res(lxres) res(lyres) res(bbp) res(dummy)
	return [array get res]
}
proc pgUpdate {} {
	global pg_request
	send_packet [pack_pgrequest 1 0 $pg_request(update)]
	array set ret [pgGetResponse]
	return $ret(data)
}
proc pgNewPopup {{width 0} {height 0}} {
	global defaultparent pg_wp
	set id [pgCreateWidget popup]
	if { $width > 0 } {
		pgSetWidget $id $pg_wp(width) $width
	}
	if { $height > 0 } {
		pgSetWidget $id $pg_wp(height) $height
	}
	if {$defaultparent == 0} {
		set defaultparent $id
	}
	return $id
}
proc pgNewPopupAt {x y width height} {
	global pg_wp
	set id [pgNewPopup $width $height]
	if { $x > -1 } {
		pgSetWidget $id $pg_wp(absolutex) $x
	}
	if { $y > -1 } {
		pgSetWidget $id $pg_wp(absolutey) $y
	}
	return $id
}
proc pgNewString {text} {
	global pg_request
	send_packet [pack_pgrequest \
		1 [string length $text] $pg_request(mkstring)]
	send_packet $text
	array set ret [pgGetResponse]
	return $ret(data)
}
proc pgGetString {textid} {
	global pg_request
	send_packet [pack_pgrequest 1 4 $pg_request(getstring)]
	send_packet [binary format "I" $textid]
	array set ret [pgGetResponse]
	return $ret(data)
}
proc pgNewWidget {type {rship 0} {parent 0}} {
	global pg_request defaultparent defaultrship pg_derive pg_widget
	if {$parent == 0 } {
		set parent $defaultparent
	}
	if {$rship == 0} {
		set rship $defaultrship
	}
	send_packet [pack_pgrequest 1 8 $pg_request(mkwidget)]
	send_packet [binary format "SSI" $rship $pg_widget($type) $parent]
	array set ret [pgGetResponse]
	set defaultparent $ret(data)
	set defaultrship $pg_derive(after)
	return $ret(data)
}
proc pgCreateWidget {type} {
	global pg_request pg_derive pg_widget
	send_packet [pack_pgrequest 1 4 $pg_request(createwidget)]
	send_packet [binary format "SS" $pg_widget($type) 0]
	array set ret [pgGetResponse]
	set defaultparent $ret(data)
	set defaultrship $pg_derive(inside)
	return $ret(data)
}
proc pgSetWidget {widget property glob} {
	global pg_request
	send_packet [pack_pgrequest 1 12 $pg_request(set)]
	send_packet [binary format "IISS" $widget $glob $property 0]
	array set ret [pgGetResponse]
	return $ret(data)
}
proc pgSetText { widget text} {
	global pg_wp
	set id [pgNewString $text]
	pgSetWidget $widget $pg_wp(text) $id
}
proc pgThemeLookup {object property} {
	global pg_request
	send_packet [pack_pgrequest 1 4 $pg_request(thlookup)]
	send_packet [binary format "SS" $object $property]
	array set ret [pgGetResponse]
	return $ret(data)
}
proc pgEnterContext {} {
	global pg_request
	send_packet [pack_pgrequest 1 0 $pg_request(mkcontext)]
	array set ret [pgGetResponse]
	return $ret(data)
}
proc pgWaitEvent {} {
	global pg_request
	send_packet [pack_pgrequest 1 0 $pg_request(wait)]
	return [pgGetResponse]
}
proc pgCheckEvent {} {
	global pg_request
	send_packet [pack_pgrequest 1 0 $pg_request(checkevent)]
	array set ret [pgGetResponse]
	return $ret(data)
}
proc pgLeaveContext {{id ""}} {
	global pg_request
	set len 0
	if { $id != "" } {
		set len 1
	}
	send_packet [pack_pgrequest 1 $len $pg_request(rmcontext)]
	if { $len < 0 } {
		send_packet [binary format "I" $id]
	}
	array set ret [pgGetResponse]
	return $ret(data)
}
proc pgFromFile { filename } {
	set data ""
	set f [open $filename]
	fconfigure $f -translation binary
	while { ![eof $f] } {
		set data [append data [read -nonewline $f]]
	}
	close $f
	return $data
}
proc pgLoadBitmap {data} {
	global pg_request
	if {[file exists $data] ==1} {
		set data [pgFromFile $data]
	}
	send_packet [pack_pgrequest 1 [string length $data] \
		$pg_request(mkbitmap)]
	send_packet $data
	array set ret [pgGetResponse]
	return $ret(data)
}
proc pgNewLabel {{text ""} {rship 0} {parent 0}} {
	set label [pgNewWidget label $rship $parent]
	pgSetText $label $text
	return $label
}
proc pgNewButton {{text ""} {rship 0} {parent 0}} {
	set button [pgNewWidget button $rship $parent]
	pgSetText $button $text
	return $button
}
proc isInteger {test} {
	set res 0
	scan $test "%d" res
	if {$test == $res} {
		return 1
	}
	return 0
}
proc pgNewBitmap {{image 0} {rship 0} {parent 0}} {
	set bitmap [pgNewWidget label $rship $parent]
	if {$image ==0} {
		return $bitmap
	} elseif {[isInteger $image] == 1} {
		pgSetBitmap $bitmap $image
	} else {
		pgSetBitmap $bitmap [pgLoadBitmap $image]
	}
	return $bitmap
}
proc pgSetBitmap {widget bitmap} {
	global pg_wp
	pgSetWidget $widget $pg_wp(bitmap) $bitmap
}
proc pgSetSide {widget side} {
	global pg_wp pg_s
	if { [isInteger $side] == 0 } {
		set side $pg_s($side)
	}
	pgSetWidget $widget $pg_wp(side) $side
}
proc pgNewFont {name style size} {
	global pg_request
	set len [string length $name]
	send_packet [pack_pgrequest 1 48 $pg_request(mkfont)]
	send_packet $name
	while {$len < 40 } {
		send_packet "\0"
		incr len
	}
	send_packet [binary format "ISS" $style $size 0]
	array set ret [pgGetResponse]
	return $ret(data)
}
proc pgSetFont {widget font} {
	global pg_wp
	pgSetWidget $widget $pg_wp(font) $font
}
proc pgRegisterApp {title type} {
	global pg_request defaultparent
	set textid [pgNewString $title]
	send_packet [pack_pgrequest 1 8 $pg_request(register)]
	send_packet [binary format "ISS" $textid $type 0]
	array set ret [pgGetResponse]
	if {$defaultparent == 0} {
		set defaultparent $ret(data)
	}
	return $ret(data)
}
proc pgBind {itemid eventid script} {
	global binds
	set indexes [array names binds]
	if {[lsearch $indexes $itemid] == -1} {
		set handlers($eventid) $script
	} else {
		array set handlers $binds($itemid)
		set handlers($eventid) $script
	}
	set binds($itemid) [array get handlers]
}
proc pgEventLoop {} {
	global binds
	while {1} {
		array set event [pgWaitEvent]
		set indexes [array names binds]
		if {[lsearch $indexes $event(from)] == -1} {
			array set handlers $binds(any)
		} else {
			array set handlers $binds($event(from))
		}
		set indexes [array names handlers]
		if {[lsearch $indexes $event(event)] == -1} {
			eval $handlers(any)
		} else {
			eval $handlers($event(event))
		}
	}
}
proc pgDialog { title } {
	global pg_widget defaultparent defaltrship
	set dlg [pgCreateWidget dialogbox]
	pgSetText $dlg $title
	set defaultparent $dlg
	return $dlg
}
