package provide picoconstants 0.4
array set pg_response {
	error	1
	ret	2
	event	3
	data	4
}
array set pg_s {
	top	8
	bottom	16
	left	32
	right	64
	all	2048
}
array set pg_derive {
	after	1
	inside	2
	before	3
}
array set pg_widget {
	toolbar		0
	label		1
	scroll		2
	indicator	3
	bitmap		1
	button		5
	panel		6
	popup		7
	box		8
	field		9
	background	10
	menuitem	11
	terminal	12
	canvas		13
	checkbox	14
	flatbutton	15
	ltstitem	16
	submenuitem	17
	radiobutton	18
	textbox		19
	panelbar	20
	simplemenu	21
}
array set pg_wp {
	size				1
	side				2
	align				3
	bgcolor				4
	color				5
	sizemod				6
	text				7
	font				8
	transparent			9
	bordercolor			10
	bitmap				12
	lgop				13
	value				14
	bitmask				15
	bind				16
	scroll_x			17
	scroll_y			18
	hotkey				19
	extdevents			20
	direction			21
	absolutx			22
	absoluty			23
	on				24
	thobj				25
	name				26
	publicbox			27
	disabled			28
	margin				29
	textformat			30
	triggermask			31
	highlighted			32
	selected			33
	selected_handle			34
	autoscroll			35
	lines				36
	preferred_w			37
	preferred_h			38
	panelbar			39
	auto_orientation		40
	thobj_button			41
	thobj_button_hilight		42
	thobj_button_on			43
	thobj_button_on_nohilight	44
	panelbar_label			45
	panelbar_close			46
	panelbar_rotate			47
	panelbar_zoom			48
	bitmapside			49
	passwd				50
	hotkey_flags			51
	hotkey_consume			52
	width				53
	height				54
	spacing				55
}
array set pg_app {
	normal	1
	toolbar	2
}
array set pg_exev {
	pntr_up		1
	pntr_down	2
	pntr_noclick	4
	pntr_move	8
	key		16
	char		32
	toggle		64
	exclusive	128
	focus		256
	no_hotspot	512
}
array set pg_eventcoding {
	param	0
	xy	256
	pntr	512
	data	768
	kbd	1024
	mask	3840
}
array set pg_request {
	update		1
	mkwidget	2
	mkbitmap	3
	mkfont		4
	mkstring	5
	set		7
	wait		13
	register	15
	mkpopup		16
	setmode		21
	getmode		22
	mkcontext	23
	rmcontext	24
	getstring	26
	thlookup	36
	checkevent	43
}
array set pg_th_o {
	label_dlgtitle	25
}
array set pg_fstyle {
	bold 256
}
array set pg_fm {
	set	0
	on	1
	off	2
	toggle	3
}
array set pg_vid {\
	rotate90	0x4
	rotate180	0x8
	rotate270	0x10
}
array set pg_we {
	activate	1
	close		3
	pntr_down	516
	pntr_up		517
	pntr_move	521
}
array set pg_trigger {
	timer		1
	pntr_relative	4
	activate	8
	deactivate	16
	keyup		32
	keydown		64
	release		128
	up		256
	down		512
	move		1024
	enter		2048
	leave		4096
	drag		8192
	char		16348
	stream		32768
	key_start	65536
	nontoolbar	131072
	pntr_status	262144
	key		524288
	scrollwheel	1048576
	touchscreen	2097152
	ts_calibrate	4194304
}
array set pg_triggers "
	mouse	[expr $pg_trigger(pntr_relative)|$pg_trigger(up)|\
		$pg_trigger(down)|$pg_trigger(move)|$pg_trigger(drag)|\
		$pg_trigger(pntr_status)|$pg_trigger(scrollwheel)|\
		$pg_trigger(release)|$pg_trigger(touchscreen)|\
		$pg_trigger(ts_calibrate)]
	key	[expr $pg_trigger(keyup)|$pg_trigger(keydown)|\
		$pg_trigger(char)|$pg_trigger(key_start)|$pg_trigger(key)]
"
parray pg_triggers
