package provide picoconstants 0.4

array set pg_request "
	update		1
	mkwidget	2
	mkbitmap	3
	mkfont		4
	mkstring	5
	free		6
	set		7
	get		8
	mktheme		9
	mkcursor	10
	mkinfliter	11
	getresource	12
	wait		13
	mkfillstyle	14
	register	15
	mkpopup		16
	sizetext	17
	batch		18
	regowner	19
	unregowner	20
	setmode		21
	getmode		22
	mkcontext	23
	rmcontext	24
	focus		25
	getstring	26
	dub		27
	setpayload	28
	getpayload	29
	chcontext	30
	writeto		31
	updatepart	32
	mkarray		33
	render		34
	newbitmap	35
	thlookup	36
	getinactive	37
	setinactive	38
	drivermsg	39
	loaddriver	40
	getfstyle	41
	findwidget	42
	checkevent	43
	sizebitmap	44
	appmsg		45
	createwidget	46
	attachwidget	47
	findthobj	48
	traverswgt	49
	mktemplate	50
	setcontext	51
	getcontext	52
	infligersend	53
	mkshmbitmap	54
"
array set pg_response "
	error	1
	ret	2
	event	3
	data	4
"
array set pgth_o "
	default				0
	base_interactive		1
	base_container			2
	button				3
	button_hilight			4
	button_on			5
	toolbar				6
	scroll				7
	scroll_hilight			8
	indicator			9
	panel				10
	panelbar			11
	popup				12
	background			13
	base_display			14
	base_tlcontainer		15
	themeinfo			16
	label				17
	field				18
	bitmap				19
	scroll_on			20
	label_scroll			21
	panelbar_hilight		22
	panelbar_on			23
	box				24
	label_dlgtitle			25
	label_dlgtext			26
	closebtn			27
	closebtn_on			28
	closebtn_hilight		29
	base_panelbtn			30
	rotatebtn			31
	rotatebtn_on			32
	rotatebtn_hilight		33
	zoombtn				34
	zoombtn_on			35
	zoombtn_hilight			36
	popup_menu			37
	popup_messagedlg		38
	menuitem			39
	menuitem_hilight		40
	checkbox			41
	checkbox_hilight		42
	chekbox_on			43
	flatbutton			44
	flatbutton_hilight		45
	flatbutton_on			46
	listitem			47
	listitem_hilight		48
	listitem_on			49
	chekbox_on_nohilight		50
	submenu				51
	submenu_hilight			52
	radiobutton			53
	radiobutton_hilight		54
	radiobutton_on			55
	radiobutton_on_nohilight	56
	textbox				57
	terminal			58
	menubutton			60
	menubutton_on			61
	menubutton_hilight		62
	label_hilight			63
	box_hilight			64
	indicator_h			65
	indicator_v			66
	custom				[expr 0x7FFF]
"
array set pg_vid "
	rotate90	[expr 0x0004]
	rotate180	[expr 0x0008]
	rotate270	[expr 0x0010]
	rotatemask	[expr 0x001C]
	rotbase90	[expr 0x0020]
	rotbase180	[expr 0x0040]
	rotbase270	[expr 0x0080]
	rotbasemask	[expr 0x00E0]
"
array set pg_we "
	activate	[expr 0x001]
	deactivate	[expr 0x002]
	close		[expr 0x003]
	focus		[expr 0x004]
	resize		[expr 0x107]
	build		[expr 0x108]
	pntr_down	[expr 0x204]
	pntr_up		[expr 0x205]
	pntr_release	[expr 0x206]
	pntr_move	[expr 0x209]
	appmsg		[expr 0x301]
	data		[expr 0x306]
	kbd_char	[expr 0x40A]
	kbd_keyup	[expr 0x40B]
	kbd_keydown	[expr 0x40C]
	
"
array set pg_widget "
	toolbar		0
	label		1
	scroll		2
	indicator	3
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
	dialogbox	22
	messagedialog	23
"
array set pg_s "
	top [expr 1<<3]
	bottom [expr 1<<4]
	left [expr 1<<5]
	right [expr 1<<6]
	all [expr 1<<11]
"
array set pg_derive "
	after	1
	inside	2
	before	3
"
array set pg_wp "
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
	absolutex			22
	absolutey			23
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
	password			50
	hotkey_flags			51
	hotkey_consume			52
	width				53
	height				54
	spacing				55
"
array set pg_fm "
	set	0
	on	1
	off	2
	toggle	3
"
array set pg_app "
	normal	1
	toolbar	2
"
array set pg_exev "
	pntr_up		[expr 0x0001]
	pntr_down	[expr 0x0002]
	pntr_noclick	[expr 0x0004]
	pntr_move	[expr 0x0008]
	key		[expr 0x0010]
	char		[expr 0x0020]
	toggle		[expr 0x0040]
	exclusive	[expr 0x0080]
	focus		[expr 0x0100]
	no_hotspot	[expr 0x0200]
"
array set pg_eventcoding "
	param	[expr 0x000]
	xy	[expr 0x100]
	pntr	[expr 0x200]
	data	[expr 0x300]
	kbd	[expr 0x400]
	mask	[expr 0xF00]
"
array set pg_fstyle "
	fixed			[expr 1<<0]
	default			[expr 1<<1]
	symbol			[expr 1<<2]
	subset			[expr 1<<3]
	encoding_isolatin1	[expr 1<<4]
	encoding_ibm		[expr 1<<5]
	doublespace		[expr 1<<7]
	bold			[expr 1<<8]
	italic			[expr 1<<9]
	underline		[expr 1<<10]
	strikeout		[expr 1<<11]
	flush			[expr 1<<14]
	doublewidth		[expr 1<<15]
	italic2			[expr 1<<16]
	encoding_unicode	[expr 1<<17]
"
set pg_fstyle(encoding_mask) [expr $pg_fstyle(encoding_isolatin1)| \
	$pg_fstyle(encoding_ibm)|$pg_fstyle(encoding_unicode)]

array set pg_trigger "
	timer		[expr 1<<0]
	pntr_relative	[expr 1<<1]
	activate	[expr 1<<3]
	deactivate	[expr 1<<4]
	keyup		[expr 1<<5]
	keydown		[expr 1<<6]
	release		[expr 1<<7]
	up		[expr 1<<8]
	down		[expr 1<<9]
	move		[expr 1<<10]
	enter		[expr 1<<11]
	leave		[expr 1<<12]
	drag		[expr 1<<13]
	char		[expr 1<<14]
	stream		[expr 1<<15]
	key_start	[expr 1<<16]
	nontoolbar	[expr 1<<17]
	pntr_status	[expr 1<<18]
	key		[expr 1<<19]
	scrollwheel	[expr 1<<20]
	touchscreen	[expr 1<<21]
	ts_calibrate	[expr 1<<22]
"
array set pg_triggers "
	mouse	[expr $pg_trigger(pntr_relative)|$pg_trigger(up)| \
		$pg_trigger(down)|$pg_trigger(move)|$pg_trigger(drag)| \
		$pg_trigger(pntr_status)|$pg_trigger(scrollwheel)| \
		$pg_trigger(release)|$pg_trigger(touchscreen)| \
		$pg_trigger(ts_calibrate)]
	key	[expr $pg_trigger(keyup)|$pg_trigger(keydown)| \
		$pg_trigger(char)|$pg_trigger(key_start)|$pg_trigger(key)]
"
