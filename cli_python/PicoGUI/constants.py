# pseudo-constant magic

#################################################################
# utility functions

def _getString(str, server, reverse=0):
    if reverse:
        if str == 0:
            return ''
        else:
            return server.getstring(str).data.replace('\0','')
    else:
        # check if it's already a handle, too
        if not server:
            return str, _getString
        try:
            str.upper()
        except:
            return str, _getString
        return server.getString(str), _getString

def _getFont(str, server, reverse=0):
    if reverse:
        return str
    else:
        if not server:
            return str, _getFont
        try:
            str.upper()
        except:
            return str, _getFont
        return server.getFont(str), _getFont

def _getBitmap(str, server, reverse=0):
    if reverse:
        return str
    else:
        return server.mkbitmap(str), _getBitmap

def _getSize(s, server, reverse=0):
    if reverse:
        return s
    else:
        # This converts fractions of the form "numerator/denominator"
        # into pgserver 8:8 bit fractions if necessary.
        # Also, allow "None" to indicate automatic sizing,
        # represented in the server by -1.
        if s == None:
            return 0xFFFFFFFFL, _getSize
        fraction = str(s).split('/')
        if len(fraction) > 1:
            return (int(fraction[0])<<8) | int(fraction[1]), _getSize
        return int(s), _getSize

#################################################################
# unique values

_stop = ((),)
_default = -1

#################################################################
# tables

_color_consts = {
            'black':            (0x000000, {}),
            'green':            (0x008000, {}),
            'silver':           (0xC0C0C0, {}),
            'lime':             (0x00FF00, {}),
            'gray':             (0x808080, {}),
            'olive':            (0x808000, {}),
            'white':            (0xFFFFFF, {}),
            'yellow':           (0xFFFF00, {}),
            'maroon':           (0x800000, {}),
            'navy':             (0x000080, {}),
            'red':              (0xFF0000, {}),
            'blue':             (0x0000FF, {}),
            'purple':           (0x800080, {}),
            'teal':             (0x008080, {}),
            'fuchsia':          (0xFF00FF, {}),
            'aqua':             (0x00FFFF, {}),
    }

_key_consts = {
  	    'backspace':	(8, {}),
	    'tab':		(0, {}),
	    'clear':		(12, {}),
	    'return':		(13, {}),
	    'pause':		(19, {}),
	    'escape':		(27, {}),
	    'space':		(32, {}),
	    'exclaim':		(33, {}),
	    'quotedbl':		(34, {}),
	    'hash':		(35, {}),
	    'dollar':		(36, {}),
	    'percent':		(37, {}),
	    'ampersand':	(38, {}),
	    'quote':		(39, {}),
	    'leftparen':	(40, {}),
	    'rightparen':	(41, {}),
	    'asterisk':		(42, {}),
	    'plus':		(43, {}),
	    'comma':		(44, {}),
	    'minus':		(45, {}),
	    'period':		(46, {}),
	    'slash':		(47, {}),
	    '0':		(48, {}),
	    '1':		(49, {}),
	    '2':		(50, {}),
	    '3':		(51, {}),
	    '4':		(52, {}),
	    '5':		(53, {}),
	    '6':		(54, {}),
	    '7':		(55, {}),
	    '8':		(56, {}),
	    '9':		(57, {}),
	    'colon':		(58, {}),
	    'semicolon':	(59, {}),
	    'greater':		(60, {}),
	    'equals':		(61, {}),
	    'greater':		(62, {}),
	    'question':		(63, {}),
	    'at':		(64, {}),
	    'leftbracket':	(91, {}),
	    'backslash':	(92, {}),
	    'rightbracket':	(93, {}),
	    'caret':		(94, {}),
	    'underscore':	(95, {}),
	    'backquote':	(96, {}),
	    'a':		(97, {}),
	    'b':		(98, {}),
	    'c':		(99, {}),
	    'd':		(100, {}),
	    'e':		(101, {}),
	    'f':		(102, {}),
	    'g':		(103, {}),
	    'h':		(104, {}),
	    'i':		(105, {}),
	    'j':		(106, {}),
	    'k':		(107, {}),
	    'l':		(108, {}),
	    'm':		(109, {}),
	    'n':		(110, {}),
	    'o':		(111, {}),
	    'p':		(112, {}),
	    'q':		(113, {}),
	    'r':		(114, {}),
	    's':		(115, {}),
	    't':		(116, {}),
	    'u':		(117, {}),
	    'v':		(118, {}),
	    'w':		(119, {}),
	    'x':		(120, {}),
	    'y':		(121, {}),
	    'z':		(122, {}),
	    'leftbrace':	(123, {}),
	    'pipe':		(124, {}),
	    'rightbrace':	(125, {}),
	    'tilde':		(126, {}),
	    'delete':		(127, {}),
	    'world_0':		(160, {}),
	    'world_1':		(161, {}),
	    'world_2':		(162, {}),
	    'world_3':		(163, {}),
	    'world_4':		(164, {}),
	    'world_5':		(165, {}),
	    'world_6':		(166, {}),
	    'world_7':		(167, {}),
	    'world_8':		(168, {}),
	    'world_9':		(169, {}),
	    'world_10':		(170, {}),
	    'world_11':		(171, {}),
	    'world_12':		(172, {}),
	    'world_13':		(173, {}),
	    'world_14':		(174, {}),
	    'world_15':		(175, {}),
	    'world_16':		(176, {}),
	    'world_17':		(177, {}),
	    'world_18':		(178, {}),
	    'world_19':		(179, {}),
	    'world_20':		(180, {}),
	    'world_21':		(181, {}),
	    'world_22':		(182, {}),
	    'world_23':		(183, {}),
	    'world_24':		(184, {}),
	    'world_25':		(185, {}),
	    'world_26':		(186, {}),
	    'world_27':		(187, {}),
	    'world_28':		(188, {}),
	    'world_29':		(189, {}),
	    'world_30':		(190, {}),
	    'world_31':		(191, {}),
	    'world_32':		(192, {}),
	    'world_33':		(193, {}),
	    'world_34':		(194, {}),
	    'world_35':		(195, {}),
	    'world_36':		(196, {}),
	    'world_37':		(197, {}),
	    'world_38':		(198, {}),
	    'world_39':		(199, {}),
	    'world_40':		(200, {}),
	    'world_41':		(201, {}),
	    'world_42':		(202, {}),
	    'world_43':		(203, {}),
	    'world_44':		(204, {}),
	    'world_45':		(205, {}),
	    'world_46':		(206, {}),
	    'world_47':		(207, {}),
	    'world_48':		(208, {}),
	    'world_49':		(209, {}),
	    'world_50':		(210, {}),
	    'world_51':		(211, {}),
	    'world_52':		(212, {}),
	    'world_53':		(213, {}),
	    'world_54':		(214, {}),
	    'world_55':		(215, {}),
	    'world_56':		(216, {}),
	    'world_57':		(217, {}),
	    'world_58':		(218, {}),
	    'world_59':		(219, {}),
	    'world_60':		(220, {}),
	    'world_61':		(221, {}),
	    'world_62':		(222, {}),
	    'world_63':		(223, {}),
	    'world_64':		(224, {}),
	    'world_65':		(225, {}),
	    'world_66':		(226, {}),
	    'world_67':		(227, {}),
	    'world_68':		(228, {}),
	    'world_69':		(229, {}),
	    'world_70':		(230, {}),
	    'world_71':		(231, {}),
	    'world_72':		(232, {}),
	    'world_73':		(233, {}),
	    'world_74':		(234, {}),
	    'world_75':		(235, {}),
	    'world_76':		(236, {}),
	    'world_77':		(237, {}),
	    'world_78':		(238, {}),
	    'world_79':		(239, {}),
	    'world_80':		(240, {}),
	    'world_81':		(241, {}),
	    'world_82':		(242, {}),
	    'world_83':		(243, {}),
	    'world_84':		(244, {}),
	    'world_85':		(245, {}),
	    'world_86':		(246, {}),
	    'world_87':		(247, {}),
	    'world_88':		(248, {}),
	    'world_89':		(249, {}),
	    'world_90':		(250, {}),
	    'world_91':		(251, {}),
	    'world_92':		(252, {}),
	    'world_93':		(253, {}),
	    'world_94':		(254, {}),
	    'world_95':		(255, {}),
	    'kp0':		(256, {}),
	    'kp1':		(257, {}),
	    'kp2':		(258, {}),
	    'kp3':		(259, {}),
	    'kp4':		(260, {}),
	    'kp5':		(261, {}),
	    'kp6':		(262, {}),
	    'kp7':		(263, {}),
	    'kp8':		(264, {}),
	    'kp9':		(265, {}),
	    'kp_period':	(266, {}),
	    'kp_divide':	(267, {}),
	    'kp_multiply':	(268, {}),
	    'kp_minus':		(269, {}),
	    'kp_plus':		(270, {}),
	    'kp_enter':		(271, {}),
	    'kp_equals':	(272, {}),
	    'up':		(273, {}),
	    'down':		(274, {}),
	    'right':		(275, {}),
	    'left':		(276, {}),
	    'insert':		(277, {}),
	    'home':		(278, {}),
	    'end':		(279, {}),
	    'pageup':		(280, {}),
	    'pagedown':		(281, {}),
	    'f1':		(282, {}),
	    'f2':		(283, {}),
	    'f3':		(284, {}),
	    'f4':		(285, {}),
	    'f5':		(286, {}),
	    'f6':		(287, {}),
	    'f7':		(288, {}),
	    'f8':		(289, {}),
	    'f9':		(290, {}),
	    'f10':		(291, {}),
	    'f11':		(292, {}),
	    'f12':		(293, {}),
	    'f13':		(294, {}),
	    'f14':		(295, {}),
	    'f15':		(296, {}),
	    'numlock':		(300, {}),
	    'capslock':		(301, {}),
	    'scrollock':	(302, {}),
	    'rshift':		(303, {}),
	    'lshift':		(304, {}),
	    'rctrl':		(305, {}),
	    'lctrl':		(306, {}),
	    'ralt':		(307, {}),
	    'lalt':		(308, {}),
	    'rmeta':		(309, {}),
	    'lmeta':		(310, {}),
	    'lsuper':		(311, {}),
	    'rsuper':		(312, {}),
	    'mode':		(313, {}),
	    'help':		(315, {}),
	    'print':		(316, {}),
	    'sysreq':		(317, {}),
	    'break':		(318, {}),
	    'menu':		(319, {}),
	    'power':		(320, {}),
	    'euro':		(321, {}),
	    'alpha':		(322, {}),
    }

_modifier_consts = {
  	    'lshift':		(0x0001, {}),
	    'rshift':		(0x0002, {}),
	    'shift':		(0x0003, {}),
	    'lctrl':		(0x0040, {}),
	    'rctrl':		(0x0080, {}),
	    'ctrl':		(0x00C0, {}),
	    'lalt':		(0x0100, {}),
	    'ralt':		(0x0200, {}),
	    'alt':		(0x0300, {}),
	    'lmeta':		(0x0400, {}),
	    'rmeta':		(0x0800, {}),
	    'meta':		(0x0C00, {}),
	    'num':		(0x1000, {}),
	    'caps':		(0x2000, {}),
	    'mode':		(0x4000, {}),
    }


_thobj_consts = {
            'default':			(0, {}),
            'base interactive':		(1, {}),
            'base container':		(2, {}),
            'button':			(3, {}),
            'button hilight':		(4, {}),
            'button on':		(5, {}),
            'toolbar':			(6, {}),
            'scroll':			(7, {}),
            'scroll hilight':		(8, {}),
            'indicator':		(9, {}),
            'panel':			(10, {}),
            'panelbar':			(11, {}),
            'popup':			(12, {}),
            'background':		(13, {}),
            'base display':		(14, {}),
            'base tlcontainer':		(15, {}),
            'themeinfo':		(16, {}),
            'label':			(17, {}),
            'field':			(18, {}),
            'bitmap':			(19, {}),
            'scroll on':		(20, {}),
            'label scroll':		(21, {}),
            'panelbar hilight':		(22, {}),
            'panelbar on':		(23, {}),
            'box':			(24, {}),
            'label dlgtitle':		(25, {}),
            'label dlgtext':		(26, {}),
            'closebtn':			(27, {}),
            'closebtn on':		(28, {}),
            'closebtn hilight':		(29, {}),
            'base panelbtn':		(30, {}),
            'rotatebtn':		(31, {}),
            'rotatebtn on':		(32, {}),
            'rotatebtn hilight':	(33, {}),
            'zoombtn':			(34, {}),
            'zoombtn on':		(35, {}),
            'zoombtn hilight':		(36, {}),
            'popup menu':		(37, {}),
            'popup messagedlg':		(38, {}),
            'menuitem':			(39, {}),
            'menuitem hilight':		(40, {}),
            'checkbox':			(41, {}),
            'checkbox hilight':		(42, {}),
            'checkbox on':		(43, {}),
            'flatbutton':		(44, {}),
            'flatbutton hilight':	(45, {}),
            'flatbutton on':		(46, {}),
            'listitem':			(47, {}),
            'listitem hilight':		(48, {}),
            'listitem on':		(49, {}),
            'checkbox on nohilight':	(50, {}),
            'submenuitem':		(51, {}),
            'submenuitem hilight':	(52, {}),
            'radiobutton':		(53, {}),
            'radiobutton hilight':	(54, {}),
            'radiobutton on':		(55, {}),
            'radiobutton on nohilight':	(56, {}),
            'textbox':			(57, {}),
            'terminal':			(58, {}),
            'menubutton':		(60, {}),
            'menubutton on':		(61, {}),
            'menubutton hilight':	(62, {}),
            'label hilight':		(63, {}),
            'box hilight':		(64, {}),
            'indicator h':		(65, {}),
            'indicator v':		(66, {}),
            'scroll h':			(67, {}),
            'scroll v':			(68, {}),
            'scroll h on':		(69, {}),
            'scroll h hilight':		(70, {}),
            'scroll v on':		(71, {}),
            'scroll v hilight':		(72, {}),
            'panelbar h':		(73, {}),
            'panelbar v':		(74, {}),
            'panelbar h on':		(75, {}),
            'panelbar h hilight':	(76, {}),
            'panelbar v on':		(77, {}),
            'panelbar v hilight':	(78, {}),
            'textedit':			(79, {}),
            'managedwindow':		(80, {}),
            'tab page':			(81, {}),
            'tab bar':			(82, {}),
            'tab':			(83, {}),
            'tab hilight':		(84, {}),
            'tab on':			(85, {}),
            'tab on nohilight':		(86, {}),
    }

_wtype_consts = {
            'toolbar':		(0, {}),
            'label':		(1, {}),
            'scroll':		(2, {}),
            'indicator':	(3, {}),
            'managedwindow':	(4, {}),
            'button':		(5, {}),
            'panel':		(6, {}),
            'popup':		(7, {}),
            'box':		(8, {}),
            'field':		(9, {}),
            'background':	(10, {}),
            'menuitem':		(11, {}),	# a variation on button
            'terminal':		(12, {}),	# a full terminal emulator
            'canvas':		(13, {}),
            'checkbox':		(14, {}),	# another variation of button
            'flatbutton':	(15, {}),	# yet another customized button
            'listitem':		(16, {}),	# still yet another...
            'submenuitem':	(17, {}),	# menuitem with a submenu arrow
            'radiobutton':	(18, {}),	# like a check box, but exclusive
            'textbox':		(19, {}),	# client-side text layout
            'panelbar':		(20, {}),	# draggable bar and container
            'simplemenu':	(21, {}),	# create a simple menu from a string or array
            'dialogbox':        (22, {}),       # a type of popup that is always autosized and has a title
            'messagedialog':    (23, {}),       # a type of dialogbox that displays a message and gets feedback
            'scrollbox':        (24, {}),       # box widget with built-in horizontal and/or vertical scrollbars
            'textedit':         (25, {}),
            'tabpage':          (26, {}),
    }

_constants = {
    'attachwidget': {
        'after':	(1, {}),
        'inside':	(2, {}),
        'before':	(3, {}),
    },
    'createwidget': _wtype_consts,
    'mkwidget': {
        'after':	(1, _wtype_consts),
        'inside':	(2, _wtype_consts),
        'before':	(3, _wtype_consts),
    },
    'get': 'set',
    'mkfont': ( # first argument (family) is string and should pass trough
        {
            'fixed':			((1<<0), {}),	# fixed width
            'default':			((1<<1), {}),	# the default font in its category, fixed or proportional.
            'symbol':			((1<<2), {}),	# font contains nonstandard chars and will not be chosen
                                                        # unless specifically requested
            'subset':			((1<<3), {}),	# font does not contain all the ascii chars before 127, and
                                                        # shouldn't be used unless requested
            'encoding isolatin1':	((1<<4), {}),	# iso latin-1 encoding
            'encoding ibm':		((1<<5), {}),	# ibm-pc extended characters
            'doublespace':		((1<<7), {}),	# add extra space between lines
            'bold':			((1<<8), {}),	# use or simulate a bold version of the font
            'italic':			((1<<9), {}),	# use or simulate an italic version of the font
            'underline':		((1<<10), {}),	# underlined text
            'strikeout':		((1<<11), {}),	# strikeout, a line through the middle of the text
            'flush':			((1<<14), {}),	# disable the margin that picogui puts around text
            'doublewidth':		((1<<15), {}),	# add extra space between characters
            'italic2':			((1<<16), {}),	# twice the slant of the default italic
            'encoding unicode':		((1<<17), {}),	# unicode encoding
    },),
    'register': _getString,
    'getresource': {
        'default font':			(0, {}),
        'string ok':			(1, {}),
        'string cancel':		(2, {}),
        'string yes':			(3, {}),
        'string no':			(4, {}),
        'string segfault':		(5, {}),
        'string matherr':		(6, {}),
        'string pguierr':		(7, {}),
        'string pguiwarn':		(8, {}),
        'string pguierrdlg':		(9, {}),
        'string pguicompat':		(10, {}),
        'default textcolors':		(11, {}),
        'infilter touchscreen': 	(12, {}),
        'infilter key preprocess':	(13, {}),
        'infilter pntr preprocess':	(14, {}),
        'infilter magic':		(15, {}),
        'infilter key dispatch':	(16, {}),
        'infilter pntr dispatch':	(17, {}),
        'default cursorbitmap': 	(18, {}),
        'default cursorbitmask': 	(19, {}),
        'background widget':    	(20, {}),
        'infilter hotspot':     	(21, {}),
        'infilter key alpha':    	(22, {}),
        'infilter pntr normalize': 	(23, {}),
    },
    'set': {
        'size':				(1,
            _getSize),
        'side':				(2, {
            'top':	(1<<3, {}),	# stick to the top edge
            'bottom':	(1<<4, {}),	# stick to the bottom edge
            'left':	(1<<5, {}),	# stick to the left edge
            'right':	(1<<6, {}),	# stick to the right edge
            'all':	(1<<11, {}),	# occupy all available space
        }),
        'align':			(3, {
            'center':	(0, {}),	# center in the available space
            'top':	(1, {}),	# stick to the top-center of the available space
            'left':	(2, {}),	# stick to the left-center of the available space
            'bottom':	(3, {}),	# stick to the bottom-center of the available space
            'right':	(4, {}),	# stick to the right-center of the available space
            'nw':	(5, {}),	# stick to the northwest corner
            'sw':	(6, {}),	# stick to the southwest corner
            'ne':	(7, {}),	# stick to the northeast corner
            'se':	(8, {}),	# stick to the southeast corner
            'all':	(9, {}),	# occupy all available space (good for tiled bitmaps)
        }),
        'bgcolor':			(4,
            _color_consts),
        'color':			(5,
            _color_consts),
        'sizemode':			(6, {
            'pixel':	(0, {}),
            'percent':	(1<<2, {}),
            'cntfrac':	(1<<15, {}),               # Container fraction
            'container fraction':  (1<<15, {}),    # Less stupidly named version of the same
        }),
        'text':				(7,
            _getString),
        'font':				(8,
            _getFont),
        'transparent':			(9, {}),
        'bordercolor':			(10,
            _color_consts),
        'image':			(12,
            _getBitmap),
        'bitmap':			'image',
        'lgop':				(13, {
        }),
        'value':			(14, {
        }),
        'bitmask':			(15,
            _getBitmap),
        'bind':				(16, {
        }),
        'scroll x':			(17, {	# horizontal and vertical scrolling amount
        }),
        'scroll y':			(18, {
        }),
        'hotkey':			(19, 
	    _key_consts),
        'extdevents':			(20, {	# mask of extra events to send
            'pointer up':      (0x0001, {}),
            'pointer down':    (0x0002, {}),
            'noclick':         (0x0004, {}),
            'pointer move':    (0x0008, {}),
            'pointer':         (0x0009, {}),
            'key':             (0x0010, {}),
            'char':            (0x0020, {}),
            'kbd':	       (0x0030, {}),
            'toggle':          (0x0040, {}),
            'exclusive':       (0x0080, {}),
            'focus':           (0x0100, {}),
            'no hotspot':      (0x0200, {}),
            'none':	       (0, {}),
            _default:	       (0, {}),
        }),
        'direction':			(21, {
            'horizontal':	(0, {}),
            'vertical':		(90, {}),
            'antihorizontal':	(180, {}),
        }),
        'absolutex':			(22, {	# read-only, relative to screen
        }),
        'absolutey':			(23, {
        }),
        'on':				(24, {  # on-off state of button/checkbox/etc
        }),
        'thobj':			(25,    # set a widget's theme object
            _thobj_consts),
        'name':				(26,    # a widget's name (for named containers, etc)
            _getString),
        'publicbox':			(27, {  # set to 1 to allow other apps to make widgets in this container
        }),
        'disabled':			(28, {  # for buttons, grays out text and prevents clicking
        }),
        'margin':			(29, {	# for boxes, overrides the default margin
        }),
        'textformat':			(30, 	# for the textbox, defines a format for text.
            _getString),
        'triggermask':			(31, {	# mask of extra triggers accepted (self->trigger_mask)
        }),
        'hilighted':			(32, {	# widget property to hilight a widget and all it's children
        }),
        'selected':			(33, {	# list property to select a row.
        }),
        'selected handle':		(34, {	# list property to return a handle to the selected row
        }),
        'autoscroll':			(35, {	# for the textbox or terminal, scroll to any new text that's inserted
        }),
        'lines':			(36, {	# height, in lines
        }),
        'preferred w':			(37, {	# read only (for now) properties to get any widget's preferred size
        }),
        'preferred h':			(38, {
        }),
        'panelbar':			(39, {	# read-only property for panels returns a handle to its embedded
                                                # panelbar widget
        }),
        'auto orientation':		(40, {	# automatically reorient child widgets when side changes
        }),
        'thobj button':			(41, 	# these four theme properties set the theme objects used for the
                                              	# three possible states of the button widget.
            _thobj_consts),

        'thobj button hilight':		(42,
            _thobj_consts),
        'thobj button on':		(43,
            _thobj_consts),
        'thobj button on nohilight':	(44,
            _thobj_consts),
        'panelbar label':		(45, {	# more read-only panelbar properties to get the built-in panelbar widgets
        }),
        'panelbar close':		(46, {
        }),
        'panelbar rotate':		(47, {
        }),
        'panelbar zoom':		(48, {
        }),
        'imageside':			(49, {
            'top':	(1<<3, {}),	# stick to the top edge
            'bottom':	(1<<4, {}),	# stick to the bottom edge
            'left':	(1<<5, {}),	# stick to the left edge
            'right':	(1<<6, {}),	# stick to the right edge
            'all':	(1<<11, {}),	# occupy all available space
        }),
        'bitmapside':			'imageside',
        'password':			(50, {
        }),
        'hotkey flags':			(51, {	# keyboard event flags for the hotkey
        }),
        'hotkey consume':		(52, {	# flag indicating whether to consume the key event when a hotkey comes in
        }),
        'width':			(53, {
        }),
        'height':			(54, {
        }),
        'spacing':			(55, {
        }),
        'minimum':			(56, {
        }),
        'multiline':			(57, {
        }),
        'selection':			(58,
            _getString),
        'readonly':			(59, {
        }),
        'insertmode':			(60, {
            'overwrite':   (0, {}),	# Replace all text
            'append':	   (1, {}),	# Add to the end
            'prepend':	   (2, {}),	# Add to the beginning
            'atcursor':	   (3, {}),	# Add at the current cursor
        }),
        'type':				(61,
            _wtype_consts),
        'tab':				(62, {
        }),
        'tab bar':			(63, {
        }),
        'popup is menu':		(63, {
        }),
        'popup is submenu':		(63, {
        }),
        'cursor position':		(63, {
        }),
        'hotkey modifiers':		(63,
	    _modifier_consts),    
    },
    'traversewidget': {
        'children':	(1, {}),	# starting with first child, traverse forward
        'forward':	(2, {}),
        'backward':	(3, {}),	# much slower than forward, avoid it
        'container':	(4, {}),	# 'count' is thenumber of container levels to traverse up
    },
    'writedata': _stop,
    'writecmd': ({
        'nuke':			(1, _stop),
        'grop':			(2, {
            'rect':		  (0x00, _stop),
            'frame':		  (0x10, _stop),
            'slab':		  (0x20, _stop),
            'bar':		  (0x30, _stop),
            'pixel':		  (0x40, _stop),
            'line':		  (0x50, _stop),
            'ellipse':		  (0x60, _stop),
            'fellipse':		  (0x70, _stop),
            'text':		  (0x04, _stop),
            'bitmap':		  (0x14, _stop),
            'tilebitmap':	  (0x24, _stop),
            'fpolygon':		  (0x34, _stop),
            'blur':		  (0x44, _stop),
            'rotatebitmap':	  (0x74, _stop),
            'resetclip':	  (0x13, _stop),
            'setoffset':	  (0x01, _stop),
            'setclip':		  (0x11, _stop),
            'setsrc':		  (0x21, _stop),
            'setmapping':	  (0x05, {
                'none':		    (0, _stop),
                'scale':	    (1, _stop),
                'squarescale':	    (2, _stop),
                'center':	    (3, _stop),
                _default:	    (0, _stop), # so you can use None
            }),
            'setcolor':		  (0x07, _stop),
            'setfont':		  (0x17, _stop),
            'setlgop':		  (0x27, {
                'null':		    (0, _stop),
                'none':		    (1, _stop),
                'or':		    (2, _stop),
                'and':		    (3, _stop),
                'xor':		    (4, _stop),
                'invert':	    (5, _stop),
                'invert or':	    (6, _stop),
                'invert and':	    (7, _stop),
                'invert xor':	    (8, _stop),
                'add':		    (9, _stop),
                'subtract':	    (10, _stop),
                'multiply':	    (11, _stop),
                'stipple':	    (12, _stop),
                'alpha':	    (13, _stop),
                _default:	    (1, _stop), # so you can use None
            }),
            'setangle':		  (0x37, _stop),
        }),
        'execfill':		(3, _stop),
        'findgrop':		(4, _stop),
        'setgrop':		(5, _stop),
        'movegrop':		(6, _stop),
        'mutategrop':		(7, _stop),
        'defaultflags':		(8, _stop),
        'gropflags':		(9, _stop),
        'redraw':		(10, _stop),
        'incremental':		(11, _stop),
        'scroll':		(12, _stop),
        'inputmapping':		(13, {
            'none':		  (0, _stop),
            'scale':		  (1, _stop),
            'squarescale':	  (2, _stop),
            'center':		  (3, _stop),
        }),
        'gridsize':		(14, _stop),
        _default:       _stop,
    },),
}

#################################################################
# some useful stuff for importing

# names of all properties
propnames = _constants['set'].keys()
# namespace for property values, to pass to resolve()
def prop_ns(name):
    return _constants['set'][name][1]

# names of all commands
cmdnames = _constants['writecmd'][0].keys()
# namespace for command arguments, to pass to resolve()
def cmd_ns(name):
    return _constants['writecmd'][0][name][1]

#################################################################
# external API

def resolve(name, namespace=_constants, server=None):
    if namespace is _stop:
        return name, namespace
    if type(namespace) == type(()):
        # for cases where the argument is really supposed to be a string, yet there are contstants after this
        return name, namespace[0]
    if callable(namespace):
        # for cases when we need even stranger processing
        return namespace(name, server)
    if type(name) in (type(()), type([])):
        # multiple names are or'ed together; namespace returned is the last one
        value = 0
        ns = namespace
        for n in name:
            v, ns = resolve(n, namespace)
            value |= v
        return value, ns
    try:
        lname = name.lower()
    except AttributeError:
        # not a string
        r = namespace.get(_default, (name, namespace))
    else:
        try:
            r = namespace[lname]
        except KeyError:
            r = namespace.get(_default, (name, namespace))
    if r is _stop:
        return name, _stop
    if type(r) == type(()) and len(r) == 2:
        if r[1]:
            return r
        else:
            # stay in the same namespace if we hit a "leaf"
            return r[0], namespace
    try:
        r.lower()
        isstr = 1
    except AttributeError:
        isstr = 0
    if isstr:
        # it's an alias
        return resolve(r, namespace)
    # otherwise, it's just a namespace
    return None, r

def unresolve(name, namespace=_constants, server=None):
    if callable(namespace):
        # If we need additional processing,
        # call the namespace with the reverse flag on
        return namespace(name, server, 1)
    for n in namespace.keys():
        if namespace[n][0] == name:
            return n
    return name
