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
	bitmap		1\
	button		5\
	panel		6\
	popup		7\
	box		8\
	field		9\
	background	10\
	menuitem	11\
	terminal	12\
	canvas		13\
	checkbox	14\
	flatbutton	15\
	ltstitem	16\
	submenuitem	17\
	radiobutton	18\
	textbox		19\
	panelbar	20\
	simplemenu	21\
}
array set pg_wp {\
	size				1\
	side				2\
	align				3\
	bgcolor				4\
	color				5\
	sizemod				6\
	text				7\
	font				8\
	transparent			9\
	bordercolor			10\
	bitmap				12\
	lgop				13\
	value				14\
	bitmask				15\
	bind				16\
	scroll_x			17\
	scroll_y			18\
	hotkey				19\
	extdevents			20\
	direction			21\
	absolutx			22\
	absoluty			23\
	on				24\
	thobj				25\
	name				26\
	publicbox			27\
	disabled			28\
	margin				29\
	textformat			30\
	triggermask			31\
	highlighted			32\
	selected			33\
	selected_handle			34\
	autoscroll			35\
	lines				36\
	preferred_w			37\
	preferred_h			38\
	panelbar			39\
	auto_orientation		40\
	thobj_button			41\
	thobj_button_hilight		42\
	thobj_button_on			43\
	thobj_button_on_nohilight	44\
	panelbar_label			45\
	panelbar_close			46\
	panelbar_rotate			47\
	panelbar_zoom			48\
	bitmapside			49\
	passwd				50\
	hotkey_flags			51\
	hotkey_consume			52\
	width				53\
	height				54\
	spacing				55\
}
array set pg_app { \
	normal	1\
	toolbar	2\
}
array set pg_request {\
	update		1\
	mkwidget	2\
	mkbitmap	3\
	mkfont		4\
	mkstring	5\
	set		7\
	wait		13\
	register	15\
	mkpopup		16\
	setmode		21\
	getmode		22\
	mkcontext	23\
	rmcontext	24\
	getstring	26\
	thlookup	36\
	checkevent	43\
}
array set pg_th_o {\
	label_dlgtitle	25\
}
array set pg_fstyle {\
	bold 256
}
array set pg_fm {\
	set	0\
	on	1\
	off	2\
	toggle	3\
}
array set pg_vid {\
	rotate90	0x4\
	rotate180	0x8\
	rotate270	0x10\
}
array set pg_we {\
	activate	1
	close		3
}

set binds(any) "any {parray event} $pg_we(close) exit"

set connection 0
set defaultparent 0
set defaultrship $pg_derive(inside)

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
proc pgNewPopupAt {x y width height} {
	global pg_request defaultparent
	send_packet [pack_pgrequest 1 8 $pg_request(mkpopup)]
	send_packet [binary format "SSSS" $x $y $width $height ]
	array set ret [pgGetResponse]
	if {$defaultparent == 0} {
		set defaultparent $ret(data)
	}
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
proc pgNewWidget {type {rship 0} {parent 0}} {
	global pg_request defaultparent defaultrship pg_derive
	if {$parent == 0 } {
		set parent $defaultparent
	}
	if {$rship == 0} {
		set rship $defaultrship
	}
	send_packet [pack_pgrequest 1 8 $pg_request(mkwidget)]
	send_packet [binary format "SSI" $rship $type $parent]
	array set ret [pgGetResponse]
	set defaultparent $ret(data)
	set defaultrship $pg_derive(after)
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
	global pg_widget
	set label [pgNewWidget $pg_widget(label) $rship $parent]
	pgSetText $label $text
	return $label
}
proc pgNewButton {{text ""} {rship 0} {parent 0}} {
	global pg_widget
	set button [pgNewWidget $pg_widget(button) $rship $parent]
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
	global pg_widget
	set bitmap [pgNewWidget $pg_widget(bitmap) $rship $parent]
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
	return ret(data)
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
