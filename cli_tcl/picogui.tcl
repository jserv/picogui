package provide picogui 0.4

if { [catch {package require picoconstants}]!=0 } {
	source picoconstants.tcl
}
set binds(any) "any {parray event} $pg_we(close) exit"

set connection 0
set defaultparent 0
set defaultrship inside

if {[info exists env(PGSERVER)]} {
	set display [split $env(PGSERVER) ":"]
} else {
	set display {"" ""}
}
set server [lindex $display 0]
set display [lindex $display 1]
if { $server == "" } {
	set server localhost
}
if {$display==""} {
	set display 0
}

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
proc pgSetWidget {widget property glob} {
	global pg_request
	send_packet [pack_pgrequest 1 12 $pg_request(set)]
	send_packet [binary format "IISS" $widget $glob $property 0]
	array set ret [pgGetResponse]
	return $ret(data)
}
proc pgGetWidget {widget property} {
	global pg_request
	send_packet [pack_pgrequest 1 8 $pg_request(set)]
	send_packet [binary format "ISS" $widget $property 0]
	array set ret [pgGetResponse]
	return $ret(data)
}
proc pgThemeLookup {object property} {
	global pg_request
	send_packet [pack_pgrequest 1 4 $pg_request(thlookup)]
	send_packet [binary format "SS" $object $property]
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
proc isInteger {test} {
	set res 0
	scan $test "%d" res
	if {$test == $res} {
		return 1
	}
	return 0
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
	pgui update
	while {1} {
		array set event [pgui waitevent]
		parray event
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
proc pgwidget {command arg1 args} {
	global pg_request pg_widget pg_derive pg_wp pg_s
	if {$command=="attach"} {
		send_packet [pack_pgrequest 1 12 $pg_request(attachwidget)]
		send_packet [binary format "IISS" [lindex $args 1] $arg1 \
			$pg_derive([lindex $args 0]) 0]
		array set ret [pgGetResponse]
	} else {
		array set aa $args
	}
	if {$command=="create"} {
		send_packet [pack_pgrequest 1 4 $pg_request(createwidget)]
		send_packet [binary format "SS" $pg_widget($arg1) 0]
		array set ret [pgGetResponse]
		return $ret(data)
	} elseif {$command=="set"} {
		foreach prop [array names aa] {
			if {$prop=="-side"} {
				pgSetWidget $arg1 $pg_wp(side) $pg_s($aa(-side))
			} elseif {$prop=="-text"} {
				set id [pgNewString $aa(-text)]
				pgSetWidget $arg1 $pg_wp(text) $id
			} elseif {$prop=="-bitmap"} {
				pgSetWidget $arg1 $pg_wp(bitmap) $aa(-bitmap)
			} else {
				puts $prop
			}
		}
	}
}
proc pgui {command args} {
	global connection pg_request
	global display server
	array set aa $args
	if {$command == "connect"} {
		if {[info exists  aa(-display)]==0} {
			set aa(-display) $display
		}
		if {[info exists aa(-server)]==0} {
			set aa(-server) $server
		}
		set port [expr 30450 + $aa(-display)] 
		if { [catch {set connection [socket $aa(-server) $port]}] != 0} {
			set connection 0
			return
		}
		fconfigure $connection -translation binary
		set data [read $connection 8]
		binary scan $data "ISS" magic protover dummy
	} elseif {$command =="entercontext"} {
		send_packet [pack_pgrequest 1 0 $pg_request(mkcontext)]
		array set ret [pgGetResponse]
	} elseif {$command =="leavecontext"} {
		set len 0
		if {[info exists aa(-id)]} {
			set len 4
		}
		send_packet [pack_pgrequest 1 $len $pg_request(rmcontext)]
		if {$len >0 } {
			send_packet [binary format "I" $aa(-id)]
		}
		array set ret [pgGetResponse]
	} elseif {$command =="checkevent"} {
		send_packet [pack_pgrequest 1 0 $pg_request(checkevent)]
		array set ret [pgGetResponse]
	} elseif {$command =="waitevent"} {
		send_packet [pack_pgrequest 1 0 $pg_request(wait)]
		array set ret [pgGetResponse]
	} elseif {$command =="update"} {
		send_packet [pack_pgrequest 1 0 $pg_request(update)]
		array set ret [pgGetResponse]
	} elseif {$command =="createbitmap"} {
		if {[info exists aa(-name)] == 1} {
			set aa(-data) [pgFromFile $aa(-name)]
		} 
		if {[info exists aa(-data)]} {
			send_packet [pack_pgrequest 1 [string length \
				$aa(-data)] $pg_request(mkbitmap)]
			send_packet $aa(-data)
			array set ret [pgGetResponse]
		} elseif {[expr [info exists aa(-width)] && [info exists aa(-height)]]} {
			send_packet [pack_pgrequest 1 4 $pg_request(newbitmap)]
			send_packet [binary format "SS" $aa(-width) $aa(-height)]
			array set ret [pgGetResponse]
		} elseif {[expr [info exists aa(-width)] || [info exists aa(-height)]]} {
			puts "wrong # args: should be pgui $command -width width -height height"
		} else {
			puts "unknown arguments for pgui createbitmap"
		}
	} else {
		puts "Unknown command $command"
		return 0
	}
	if {[info exists ret(data)]} {
		return $ret(data)
	} else {
		return [array get ret]
	}
}
