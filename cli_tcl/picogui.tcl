array set pg_response {\
	error	1\
	ret	2\
	event	3\
	data	4\
}
array set pg_s {\
	top	8\
	bottom	16\
	left	32\
	right	64\
	all	2048\
}
array set pg_derive {\
	after	1\
	inside	2\
	before	3\
}
array set pg_widget {\
	toolbar		0\
	label		1\
	scroll		2\
	indicator	3\
	bitmap		4\
	button		5\
	panel		6\
	popup		7\
	box		8\
	field		9\
	background	10\
	menuitem	11\
	terminal	12\
	canvas		13\
	checkbutton	14\
	flatbutton	15\
	ltstitem	16\
	submenuitem	17\
	radiobutton	18\
	textbox		19\
	panelbar	20\
}
array set pg_request {\
	update		1\
	mkwidget	2\
	mkbitmap	3\
	mkstring	5\
	set		7\
	wait		13\
	mkpopup		16\
	getmode		22\
	mkcontext	23\
	rmcontext	24\
	getstring	26\
	thlookup	36\
	checkevent	43\
}
array set pg_wp {\
	side		2\
	text		7\
	transparent	9\
	bitmap		12\
	thobj		25\
}
array set pg_th_o {
	label_dlgtitle	25\
}

set connection 0

proc pgGetResponse {} {
	global pg_response pg_request connection
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
}
proc send_packet {packet} {
	global connection
	puts -nonewline $connection $packet
	flush $connection
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
proc pgNewPopupAt {x y width height} {
	global pg_request
	send_packet [pack_pgrequest 1 8 $pg_request(mkpopup)]
	send_packet [binary format "SSSS" $x $y $width $height ]
	array set ret [pgGetResponse]
	return $ret(data)
}
proc pgNewPopup {width height} {
	return [pgNewPopupAt -1 -1 $width $height]
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
proc pgNewWidget {rship type parent} {
	global pg_request
	send_packet [pack_pgrequest 1 8 $pg_request(mkwidget)]
	send_packet [binary format "SSI" $rship $type $parent]
	array set ret [pgGetResponse]
	return $ret(data)
}
proc pgSetWidget {widget glob property} {
	global pg_request
	send_packet [pack_pgrequest 1 12 $pg_request(set)]
	send_packet [binary format "IISS" $widget $glob $property 0]
	array set ret [pgGetResponse]
	return $ret(data)
}
proc pgNewLabel {rship parent {text ""}} {
	global pg_wp pg_widget
	set id [pgNewString $text]
	set label [pgNewWidget $rship $pg_widget(label) $parent]
	pgSetWidget $label $id $pg_wp(text)
	return $label
}
proc pgDialog {title} {
	global pg_derive pg_wp pg_th_o pg_s label
	set popup [pgNewPopup 0 0]
	set label [pgNewLabel $pg_derive(inside) $popup $title]
	pgSetWidget $label 0 $pg_wp(transparent)
	pgSetWidget $label $pg_th_o(label_dlgtitle) $pg_wp(thobj)
	return $popup
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
proc pgNewBitmap {data} {
	global pg_request
	send_packet [pack_pgrequest 1 [string length $data] \
		$pg_request(mkbitmap)]
	send_packet $data
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
