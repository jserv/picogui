set pg_response(ret)	2
set pg_response(event)	3
set pg_response(data)	4

set pg_widget(LABEL)	1
set pg_widget(BITMAP)	4
set pg_widget(BUTTON)	5

set pg_request(PGREQ_UPDATE)		1
set pg_request(PGREQ_MKWIDGET)		2
set pg_request(PGREQ_MKBITMAP)		3
set pg_request(PGREQ_MKSTRING)		5
set pg_request(PGREQ_SET)		7
set pg_request(PGREQ_WAIT)		13
set pg_request(PGREQ_MKPOPUP)		16
set pg_request(PGREQ_GETMODE)		22
set pg_request(PGREQ_MKCONTEXT)		23
set pg_request(PGREQ_RMCONTEXT)		24
set pg_request(PGREQ_GETSTRING)		26
set pg_request(PGREQ_THLOOKUP)		36
set pg_request(PGREQ_CHECKEVENT)	43

set pg_wp(SIDE)		2
set pg_wp(TEXT)		7
set pg_wp(TRANSPARENT)	9
set pg_wp(BITMAP)	12
set pg_wp(THOBJ)	25

set pg_th_o(LABEL_DLGTITLE)	25

set pg_derive(after)	1
set pg_derive(inside)	2
set pg_derive(before)	3

set pg_s(TOP)		8
set pg_s(BOTTOM)	16
set pg_s(LEFT)		32
set pg_s(RIGHT)		64
set pg_s(ALL)		2048

set connection 0

proc get_response {} {
	global pg_response pg_request connection
	set data [read $connection 2]
	binary scan $data "S" resp
	return $resp
}
proc pack_pgrequest { id size type {dummy 0} } {
	set req [binary format "IISS" $id $size $type $dummy]
	return $req
}
proc connect {server display} {
	global connection
	set port [expr 30450 + $display] 
	if { [catch {set sock [socket $server $port]}] != 0 } {
		return
	}
	set data [read $sock 8]
	binary scan $data "ISS" magic protover dummy
	fconfigure $sock -translation binary
	set connection $sock
	return $sock
}
proc send_packet {packet} {
	global connection
	puts -nonewline $connection $packet
	flush $connection
}
proc pgGetVideoMode {} {
	global pg_request pg_response connection
	send_packet [pack_pgrequest 1 0 $pg_request(PGREQ_GETMODE)]
	if { [get_response] == $pg_response(data) } {
		set data [read $connection 10]
		binary scan $data "SII" dummy id size
		set data [read $connection 16]
		binary scan $data "ISSSSSS" res(flags) \
			res(xres) res(yres) \
			res(lxres) res(lyres) res(bbp) res(dummy)
	}
	return [array get res]
}
proc pgReturnResponse {} {
	global pg_response connection
	set resp [get_response]
	if { $resp == $pg_response(ret)} {
		set data [read $connection 10]
		binary scan $data "SII" dummy id data
		return $data
	}
}
proc pgUpdate {} {
	global pg_request pg_response
	send_packet [pack_pgrequest 1 0 $pg_request(PGREQ_UPDATE)]
	return [pgReturnResponse]
}
proc pgNewPopupAt {x y width height} {
	global pg_request pg_response
	send_packet [pack_pgrequest 1 8 $pg_request(PGREQ_MKPOPUP)]
	send_packet [binary format "SSSS" $x $y $width $height ]
	return [pgReturnResponse]
}
proc pgNewPopup {width height} {
	return [pgNewPopupAt -1 -1 $width $height]
}
proc pgNewString {text} {
	global pg_request
	send_packet [pack_pgrequest \
		1 [string length $text] $pg_request(PGREQ_MKSTRING)]
	send_packet $text
	return [pgReturnResponse]
}
proc pgGetString {text} {
	global pg_request pg_response connection
	send_packet [pack_pgrequest 1 4 $pg_request(PGREQ_GETSTRING)]
	send_packet [binary format "I" $text]
	set resp [get_response]
	if { $resp == $pg_response(data) } {
		set data [read $connection 10]
		binary scan $data "SII" dummy id size
		set data [read $connection $size]
		return $data
	}
}
proc pgNewWidget {rship type parent} {
	global pg_request
	send_packet [pack_pgrequest 1 8 $pg_request(PGREQ_MKWIDGET)]
	send_packet [binary format "SSI" $rship $type $parent]
	return [pgReturnResponse]
}
proc pgSetWidget {widget glob property} {
	global pg_request
	send_packet [pack_pgrequest 1 12 $pg_request(PGREQ_SET)]
	send_packet [binary format "IISS" $widget $glob $property 0]
	return [pgReturnResponse]
}
proc pgNewLabel {rship parent {text ""}} {
	global pg_wp pg_widget
	set id [pgNewString $text]
	set label [pgNewWidget $rship $pg_widget(LABEL) $parent]
	pgSetWidget $label $id $pg_wp(TEXT)
	return $label
}
proc pgDialog {title} {
	global pg_derive pg_wp pg_th_o pg_s label
	set popup [pgNewPopup 0 0]
	set label [pgNewLabel $pg_derive(inside) $popup $title]
	pgSetWidget $label 0 $pg_wp(TRANSPARENT)
	pgSetWidget $label $pg_th_o(LABEL_DLGTITLE) $pg_wp(THOBJ)
	return $popup
}
proc pgThemeLookup {object property} {
	global pg_request
	send_packet [pack_pgrequest 1 4 $pg_request(PGREQ_THLOOKUP)]
	send_packet [binary format "SS" $object $property]
	return [pgReturnResponse]
}
proc pgEnterContext {} {
	global pg_request
	send_packet [pack_pgrequest 1 0 $pg_request(PGREQ_MKCONTEXT)]
	return [pgReturnResponse]
}
proc pgWaitEvent {} {
	global pg_request pg_response connection
	send_packet [pack_pgrequest 1 0 $pg_request(PGREQ_WAIT)]
	set resp [get_response]
	if { $resp == $pg_response(event) } {
		set data [read $connection 10]
		binary scan $data "SII" ret(event) ret(from) ret(param)
		return [array get ret]
	}
}
proc pgCheckEvent {} {
	global pg_request
	send_packet [pack_pgrequest 1 0 $pg_request(PGREQ_CHECKEVENT)]
	return [pgReturnResponse]
}
proc pgLeaveContext {{id ""}} {
	global pg_request
	set len 0
	if { $id != "" } {
		set len 1
	}
	send_packet [pack_pgrequest 1 $len $pg_request(PGREQ_RMCONTEXT)]
	if { $len < 0 } {
		send_packet [binary format "I" $id]
	}
	return [pgReturnResponse]
}
proc pgNewBitmap {data} {
	global pg_request
	send_packet [pack_pgrequest 1 [string length $data] \
		$pg_request(PGREQ_MKBITMAP)]
	send_packet $data
	return [pgReturnResponse]
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
