package PicoGUI;

require 5.005_62;
use strict;
use warnings;
use Carp;

require Exporter;
require DynaLoader;
use AutoLoader;

our @ISA = qw(Exporter DynaLoader);

# Items to export into callers namespace by default. Note: do not export
# names by default without a very good reason. Use EXPORT_OK instead.
# Do not simply export all your public functions/methods/constants.

# This allows declaration	use PicoGUI ':all';
# If you do not need this, moving things directly into @EXPORT or @EXPORT_OK
# will save memory.
our %EXPORT_TAGS = ( 'all' => [ qw(
	NULL
	PGBIND_ANY
	PGCANVAS_COLORCONV
	PGCANVAS_EXECFILL
	PGCANVAS_FINDGROP
	PGCANVAS_GROP
	PGCANVAS_GROPFLAGS
	PGCANVAS_INCREMENTAL
	PGCANVAS_MOVEGROP
	PGCANVAS_MUTATEGROP
	PGCANVAS_NUKE
	PGCANVAS_REDRAW
	PGCANVAS_SCROLL
	PGCANVAS_SETGROP
	PGDEFAULT
	PGFONT_ANY
	PGKEY_0
	PGKEY_1
	PGKEY_2
	PGKEY_3
	PGKEY_4
	PGKEY_5
	PGKEY_6
	PGKEY_7
	PGKEY_8
	PGKEY_9
	PGKEY_AMPERSAND
	PGKEY_ASTERISK
	PGKEY_AT
	PGKEY_BACKQUOTE
	PGKEY_BACKSLASH
	PGKEY_BACKSPACE
	PGKEY_BREAK
	PGKEY_CAPSLOCK
	PGKEY_CARET
	PGKEY_CLEAR
	PGKEY_COLON
	PGKEY_COMMA
	PGKEY_DELETE
	PGKEY_DOLLAR
	PGKEY_DOWN
	PGKEY_END
	PGKEY_EQUALS
	PGKEY_ESCAPE
	PGKEY_EURO
	PGKEY_EXCLAIM
	PGKEY_F1
	PGKEY_F10
	PGKEY_F11
	PGKEY_F12
	PGKEY_F13
	PGKEY_F14
	PGKEY_F15
	PGKEY_F2
	PGKEY_F3
	PGKEY_F4
	PGKEY_F5
	PGKEY_F6
	PGKEY_F7
	PGKEY_F8
	PGKEY_F9
	PGKEY_GREATER
	PGKEY_HASH
	PGKEY_HELP
	PGKEY_HOME
	PGKEY_INSERT
	PGKEY_KP0
	PGKEY_KP1
	PGKEY_KP2
	PGKEY_KP3
	PGKEY_KP4
	PGKEY_KP5
	PGKEY_KP6
	PGKEY_KP7
	PGKEY_KP8
	PGKEY_KP9
	PGKEY_KP_DIVIDE
	PGKEY_KP_ENTER
	PGKEY_KP_EQUALS
	PGKEY_KP_MINUS
	PGKEY_KP_MULTIPLY
	PGKEY_KP_PERIOD
	PGKEY_KP_PLUS
	PGKEY_LALT
	PGKEY_LCTRL
	PGKEY_LEFT
	PGKEY_LEFTBRACKET
	PGKEY_LEFTPAREN
	PGKEY_LESS
	PGKEY_LMETA
	PGKEY_LSHIFT
	PGKEY_LSUPER
	PGKEY_MENU
	PGKEY_MINUS
	PGKEY_MODE
	PGKEY_NUMLOCK
	PGKEY_PAGEDOWN
	PGKEY_PAGEUP
	PGKEY_PAUSE
	PGKEY_PERIOD
	PGKEY_PLUS
	PGKEY_POWER
	PGKEY_PRINT
	PGKEY_QUESTION
	PGKEY_QUOTE
	PGKEY_QUOTEDBL
	PGKEY_RALT
	PGKEY_RCTRL
	PGKEY_RETURN
	PGKEY_RIGHT
	PGKEY_RIGHTBRACKET
	PGKEY_RIGHTPAREN
	PGKEY_RMETA
	PGKEY_RSHIFT
	PGKEY_RSUPER
	PGKEY_SCROLLOCK
	PGKEY_SEMICOLON
	PGKEY_SLASH
	PGKEY_SPACE
	PGKEY_SYSREQ
	PGKEY_TAB
	PGKEY_UNDERSCORE
	PGKEY_UP
	PGKEY_WORLD_0
	PGKEY_WORLD_1
	PGKEY_WORLD_10
	PGKEY_WORLD_11
	PGKEY_WORLD_12
	PGKEY_WORLD_13
	PGKEY_WORLD_14
	PGKEY_WORLD_15
	PGKEY_WORLD_16
	PGKEY_WORLD_17
	PGKEY_WORLD_18
	PGKEY_WORLD_19
	PGKEY_WORLD_2
	PGKEY_WORLD_20
	PGKEY_WORLD_21
	PGKEY_WORLD_22
	PGKEY_WORLD_23
	PGKEY_WORLD_24
	PGKEY_WORLD_25
	PGKEY_WORLD_26
	PGKEY_WORLD_27
	PGKEY_WORLD_28
	PGKEY_WORLD_29
	PGKEY_WORLD_3
	PGKEY_WORLD_30
	PGKEY_WORLD_31
	PGKEY_WORLD_32
	PGKEY_WORLD_33
	PGKEY_WORLD_34
	PGKEY_WORLD_35
	PGKEY_WORLD_36
	PGKEY_WORLD_37
	PGKEY_WORLD_38
	PGKEY_WORLD_39
	PGKEY_WORLD_4
	PGKEY_WORLD_40
	PGKEY_WORLD_41
	PGKEY_WORLD_42
	PGKEY_WORLD_43
	PGKEY_WORLD_44
	PGKEY_WORLD_45
	PGKEY_WORLD_46
	PGKEY_WORLD_47
	PGKEY_WORLD_48
	PGKEY_WORLD_49
	PGKEY_WORLD_5
	PGKEY_WORLD_50
	PGKEY_WORLD_51
	PGKEY_WORLD_52
	PGKEY_WORLD_53
	PGKEY_WORLD_54
	PGKEY_WORLD_55
	PGKEY_WORLD_56
	PGKEY_WORLD_57
	PGKEY_WORLD_58
	PGKEY_WORLD_59
	PGKEY_WORLD_6
	PGKEY_WORLD_60
	PGKEY_WORLD_61
	PGKEY_WORLD_62
	PGKEY_WORLD_63
	PGKEY_WORLD_64
	PGKEY_WORLD_65
	PGKEY_WORLD_66
	PGKEY_WORLD_67
	PGKEY_WORLD_68
	PGKEY_WORLD_69
	PGKEY_WORLD_7
	PGKEY_WORLD_70
	PGKEY_WORLD_71
	PGKEY_WORLD_72
	PGKEY_WORLD_73
	PGKEY_WORLD_74
	PGKEY_WORLD_75
	PGKEY_WORLD_76
	PGKEY_WORLD_77
	PGKEY_WORLD_78
	PGKEY_WORLD_79
	PGKEY_WORLD_8
	PGKEY_WORLD_80
	PGKEY_WORLD_81
	PGKEY_WORLD_82
	PGKEY_WORLD_83
	PGKEY_WORLD_84
	PGKEY_WORLD_85
	PGKEY_WORLD_86
	PGKEY_WORLD_87
	PGKEY_WORLD_88
	PGKEY_WORLD_89
	PGKEY_WORLD_9
	PGKEY_WORLD_90
	PGKEY_WORLD_91
	PGKEY_WORLD_92
	PGKEY_WORLD_93
	PGKEY_WORLD_94
	PGKEY_WORLD_95
	PGKEY_a
	PGKEY_b
	PGKEY_c
	PGKEY_d
	PGKEY_e
	PGKEY_f
	PGKEY_g
	PGKEY_h
	PGKEY_i
	PGKEY_j
	PGKEY_k
	PGKEY_l
	PGKEY_m
	PGKEY_n
	PGKEY_o
	PGKEY_p
	PGKEY_q
	PGKEY_r
	PGKEY_s
	PGKEY_t
	PGKEY_u
	PGKEY_v
	PGKEY_w
	PGKEY_x
	PGKEY_y
	PGKEY_z
	PGMEMDAT_NEED_FREE
	PGMEMDAT_NEED_UNMAP
	PGMOD_ALT
	PGMOD_CAPS
	PGMOD_CTRL
	PGMOD_LALT
	PGMOD_LCTRL
	PGMOD_LMETA
	PGMOD_LSHIFT
	PGMOD_META
	PGMOD_MODE
	PGMOD_NUM
	PGMOD_RALT
	PGMOD_RCTRL
	PGMOD_RMETA
	PGMOD_RSHIFT
	PGMOD_SHIFT
	PGREQ_BATCH
	PGREQ_FOCUS
	PGREQ_FREE
	PGREQ_GET
	PGREQ_GETMODE
	PGREQ_GETPAYLOAD
	PGREQ_GETSTRING
	PGREQ_IN_DIRECT
	PGREQ_IN_KEY
	PGREQ_IN_POINT
	PGREQ_MKBITMAP
	PGREQ_MKCONTEXT
	PGREQ_MKFILLSTYLE
	PGREQ_MKFONT
	PGREQ_MKMENU
	PGREQ_MKMSGDLG
	PGREQ_MKPOPUP
	PGREQ_MKSTRING
	PGREQ_MKTHEME
	PGREQ_MKWIDGET
	PGREQ_PING
	PGREQ_REGISTER
	PGREQ_REGOWNER
	PGREQ_RMCONTEXT
	PGREQ_SET
	PGREQ_SETMODE
	PGREQ_SETPAYLOAD
	PGREQ_SIZETEXT
	PGREQ_UNDEF
	PGREQ_UNREGOWNER
	PGREQ_UPDATE
	PGREQ_UPDATEPART
	PGREQ_WAIT
	PGREQ_WRITETO
	PGTH_FORMATVERSION
	PGTH_LOAD_COPY
	PGTH_LOAD_NONE
	PGTH_LOAD_REQUEST
	PGTH_ONUM
	PGTH_OPCMD_AND
	PGTH_OPCMD_COLOR
	PGTH_OPCMD_COLORADD
	PGTH_OPCMD_COLORDIV
	PGTH_OPCMD_COLORMULT
	PGTH_OPCMD_COLORSUB
	PGTH_OPCMD_DIVIDE
	PGTH_OPCMD_EQ
	PGTH_OPCMD_GT
	PGTH_OPCMD_LOCALPROP
	PGTH_OPCMD_LOGICAL_AND
	PGTH_OPCMD_LOGICAL_NOT
	PGTH_OPCMD_LOGICAL_OR
	PGTH_OPCMD_LONGGET
	PGTH_OPCMD_LONGGROP
	PGTH_OPCMD_LONGLITERAL
	PGTH_OPCMD_LONGSET
	PGTH_OPCMD_LT
	PGTH_OPCMD_MINUS
	PGTH_OPCMD_MULTIPLY
	PGTH_OPCMD_OR
	PGTH_OPCMD_PLUS
	PGTH_OPCMD_PROPERTY
	PGTH_OPCMD_QUESTIONCOLON
	PGTH_OPCMD_SHIFTL
	PGTH_OPCMD_SHIFTR
	PGTH_OPSIMPLE_CMDCODE
	PGTH_OPSIMPLE_GET
	PGTH_OPSIMPLE_GROP
	PGTH_OPSIMPLE_LITERAL
	PGTH_OPSIMPLE_SET
	PGTH_O_BACKGROUND
	PGTH_O_BASE_CONTAINER
	PGTH_O_BASE_DISPLAY
	PGTH_O_BASE_INTERACTIVE
	PGTH_O_BASE_PANELBTN
	PGTH_O_BASE_TLCONTAINER
	PGTH_O_BITMAP
	PGTH_O_BOX
	PGTH_O_BUTTON
	PGTH_O_BUTTON_HILIGHT
	PGTH_O_BUTTON_ON
	PGTH_O_CHECKBOX
	PGTH_O_CHECKBOX_HILIGHT
	PGTH_O_CHECKBOX_ON
	PGTH_O_CLOSEBTN
	PGTH_O_CLOSEBTN_HILIGHT
	PGTH_O_CLOSEBTN_ON
	PGTH_O_DEFAULT
	PGTH_O_FIELD
	PGTH_O_FLATBUTTON
	PGTH_O_FLATBUTTON_HILIGHT
	PGTH_O_FLATBUTTON_ON
	PGTH_O_INDICATOR
	PGTH_O_LABEL
	PGTH_O_LABEL_DLGTEXT
	PGTH_O_LABEL_DLGTITLE
	PGTH_O_LABEL_SCROLL
	PGTH_O_MENUITEM
	PGTH_O_MENUITEM_HILIGHT
	PGTH_O_PANEL
	PGTH_O_PANELBAR
	PGTH_O_PANELBAR_HILIGHT
	PGTH_O_PANELBAR_ON
	PGTH_O_POPUP
	PGTH_O_POPUP_MENU
	PGTH_O_POPUP_MESSAGEDLG
	PGTH_O_ROTATEBTN
	PGTH_O_ROTATEBTN_HILIGHT
	PGTH_O_ROTATEBTN_ON
	PGTH_O_SCROLL
	PGTH_O_SCROLL_HILIGHT
	PGTH_O_SCROLL_ON
	PGTH_O_THEMEINFO
	PGTH_O_TOOLBAR
	PGTH_O_ZOOMBTN
	PGTH_O_ZOOMBTN_HILIGHT
	PGTH_O_ZOOMBTN_ON
	PGTH_P_ALIGN
	PGTH_P_BACKDROP
	PGTH_P_BGCOLOR
	PGTH_P_BGFILL
	PGTH_P_BITMAP1
	PGTH_P_BITMAP2
	PGTH_P_BITMAP3
	PGTH_P_BITMAP4
	PGTH_P_BITMAPMARGIN
	PGTH_P_BITMAPSIDE
	PGTH_P_CURSORBITMAP
	PGTH_P_CURSORBITMASK
	PGTH_P_FGCOLOR
	PGTH_P_FONT
	PGTH_P_HEIGHT
	PGTH_P_HILIGHTCOLOR
	PGTH_P_HOTKEY_CANCEL
	PGTH_P_HOTKEY_NO
	PGTH_P_HOTKEY_OK
	PGTH_P_HOTKEY_YES
	PGTH_P_ICON_CANCEL
	PGTH_P_ICON_OK
	PGTH_P_MARGIN
	PGTH_P_NAME
	PGTH_P_OFFSET
	PGTH_P_OVERLAY
	PGTH_P_SHADOWCOLOR
	PGTH_P_SIDE
	PGTH_P_SPACING
	PGTH_P_STRING_CANCEL
	PGTH_P_STRING_NO
	PGTH_P_STRING_OK
	PGTH_P_STRING_YES
	PGTH_P_TEXT
	PGTH_P_WIDGETBITMAP
	PGTH_P_WIDGETBITMASK
	PGTH_P_WIDTH
	PGTH_TAG_AUTHOR
	PGTH_TAG_AUTHOREMAIL
	PGTH_TAG_README
	PGTH_TAG_URL
	PG_AMAX
	PG_APPMAX
	PG_APPSPEC_HEIGHT
	PG_APPSPEC_MAXHEIGHT
	PG_APPSPEC_MAXWIDTH
	PG_APPSPEC_MINHEIGHT
	PG_APPSPEC_MINWIDTH
	PG_APPSPEC_SIDE
	PG_APPSPEC_SIDEMASK
	PG_APPSPEC_WIDTH
	PG_APP_NORMAL
	PG_APP_TOOLBAR
	PG_A_ALL
	PG_A_BOTTOM
	PG_A_CENTER
	PG_A_LEFT
	PG_A_NE
	PG_A_NW
	PG_A_RIGHT
	PG_A_SE
	PG_A_SW
	PG_A_TOP
	PG_DERIVE_AFTER
	PG_DERIVE_BEFORE
	PG_DERIVE_BEFORE_OLD
	PG_DERIVE_INSIDE
	PG_DIR_HORIZONTAL
	PG_DIR_VERTICAL
	PG_ERRT_BADPARAM
	PG_ERRT_BUSY
	PG_ERRT_CLIENT
	PG_ERRT_FILEFMT
	PG_ERRT_HANDLE
	PG_ERRT_INTERNAL
	PG_ERRT_IO
	PG_ERRT_MEMORY
	PG_ERRT_NETWORK
	PG_ERRT_NONE
	PG_EVENTCODINGMASK
	PG_EVENTCODING_DATA
	PG_EVENTCODING_KBD
	PG_EVENTCODING_PARAM
	PG_EVENTCODING_PNTR
	PG_EVENTCODING_XY
	PG_EXEV_CHAR
	PG_EXEV_KEY
	PG_EXEV_NOCLICK
	PG_EXEV_PNTR_DOWN
	PG_EXEV_PNTR_MOVE
	PG_EXEV_PNTR_UP
	PG_EXEV_TOGGLE
	PG_FM_OFF
	PG_FM_ON
	PG_FM_SET
	PG_FM_TOGGLE
	PG_FSTYLE_BOLD
	PG_FSTYLE_DEFAULT
	PG_FSTYLE_DOUBLESPACE
	PG_FSTYLE_DOUBLEWIDTH
	PG_FSTYLE_EXTENDED
	PG_FSTYLE_FIXED
	PG_FSTYLE_FLUSH
	PG_FSTYLE_GRAYLINE
	PG_FSTYLE_IBMEXTEND
	PG_FSTYLE_ITALIC
	PG_FSTYLE_ITALIC2
	PG_FSTYLE_STRIKEOUT
	PG_FSTYLE_SUBSET
	PG_FSTYLE_SYMBOL
	PG_FSTYLE_UNDERLINE
	PG_GROPF_INCREMENTAL
	PG_GROPF_PSEUDOINCREMENTAL
	PG_GROPF_TRANSLATE
	PG_GROP_BAR
	PG_GROP_BITMAP
	PG_GROP_DIM
	PG_GROP_FRAME
	PG_GROP_GRADIENT
	PG_GROP_LINE
	PG_GROP_NULL
	PG_GROP_PIXEL
	PG_GROP_RECT
	PG_GROP_SLAB
	PG_GROP_TEXT
	PG_GROP_TEXTGRID
	PG_GROP_TEXTV
	PG_GROP_TILEBITMAP
	PG_LGOPMAX
	PG_LGOP_AND
	PG_LGOP_INVERT
	PG_LGOP_INVERT_AND
	PG_LGOP_INVERT_OR
	PG_LGOP_INVERT_XOR
	PG_LGOP_NONE
	PG_LGOP_NULL
	PG_LGOP_OR
	PG_LGOP_XOR
	PG_MAX_RESPONSE_SZ
	PG_MSGBTN_CANCEL
	PG_MSGBTN_NO
	PG_MSGBTN_OK
	PG_MSGBTN_YES
	PG_NWE_BGCLICK
	PG_NWE_KBD_CHAR
	PG_NWE_KBD_KEYDOWN
	PG_NWE_KBD_KEYUP
	PG_NWE_PNTR_DOWN
	PG_NWE_PNTR_MOVE
	PG_NWE_PNTR_UP
	PG_OWN_KEYBOARD
	PG_OWN_POINTER
	PG_OWN_SYSEVENTS
	PG_POPUP_ATCURSOR
	PG_POPUP_CENTER
	PG_PROTOCOL_VER
	PG_REQUEST_MAGIC
	PG_REQUEST_PORT
	PG_RESPONSE_DATA
	PG_RESPONSE_ERR
	PG_RESPONSE_EVENT
	PG_RESPONSE_RET
	PG_SZMODEMASK
	PG_SZMODE_CNTFRACT
	PG_SZMODE_PERCENT
	PG_SZMODE_PIXEL
	PG_S_ALL
	PG_S_BOTTOM
	PG_S_LEFT
	PG_S_RIGHT
	PG_S_TOP
	PG_TRIGGER_CHAR
	PG_TRIGGER_DOWN
	PG_TRIGGER_KEYDOWN
	PG_TRIGGER_KEYUP
	PG_TRIGGER_MOVE
	PG_TRIGGER_UP
	PG_TYPE_BITMAP
	PG_TYPE_FILLSTYLE
	PG_TYPE_FONTDESC
	PG_TYPE_STRING
	PG_TYPE_THEME
	PG_TYPE_WIDGET
	PG_VID_DOUBLEBUFFER
	PG_VID_FULLSCREEN
	PG_VID_ROTATE90
	PG_WE_ACTIVATE
	PG_WE_BUILD
	PG_WE_CLOSE
	PG_WE_DATA
	PG_WE_DEACTIVATE
	PG_WE_KBD_CHAR
	PG_WE_KBD_KEYDOWN
	PG_WE_KBD_KEYUP
	PG_WE_PNTR_DOWN
	PG_WE_PNTR_MOVE
	PG_WE_PNTR_UP
	PG_WE_RESIZE
	PG_WIDGETMAX
	PG_WIDGET_BACKGROUND
	PG_WIDGET_BITMAP
	PG_WIDGET_BOX
	PG_WIDGET_BUTTON
	PG_WIDGET_CANVAS
	PG_WIDGET_CHECKBOX
	PG_WIDGET_FIELD
	PG_WIDGET_FLATBUTTON
	PG_WIDGET_INDICATOR
	PG_WIDGET_LABEL
	PG_WIDGET_MENUITEM
	PG_WIDGET_PANEL
	PG_WIDGET_POPUP
	PG_WIDGET_SCROLL
	PG_WIDGET_TERMINAL
	PG_WIDGET_TOOLBAR
	PG_WP_ABSOLUTEX
	PG_WP_ABSOLUTEY
	PG_WP_ALIGN
	PG_WP_BGCOLOR
	PG_WP_BIND
	PG_WP_BITMAP
	PG_WP_BITMASK
	PG_WP_BORDERCOLOR
	PG_WP_COLOR
	PG_WP_DIRECTION
	PG_WP_EXTDEVENTS
	PG_WP_FONT
	PG_WP_HOTKEY
	PG_WP_LGOP
	PG_WP_ON
	PG_WP_SCROLL
	PG_WP_SIDE
	PG_WP_SIZE
	PG_WP_SIZEMODE
	PG_WP_TEXT
	PG_WP_TRANSPARENT
	PG_WP_VALUE
	PG_WP_VIRTUALH
	_H_PG_CANVAS
	_H_PG_CLI_C
	_H_PG_CONSTANTS
	_H_PG_NETWORK
) ] );

our @EXPORT_OK = ( @{ $EXPORT_TAGS{'all'} } );

our @EXPORT = qw(
	NULL
	PGBIND_ANY
	PGCANVAS_COLORCONV
	PGCANVAS_EXECFILL
	PGCANVAS_FINDGROP
	PGCANVAS_GROP
	PGCANVAS_GROPFLAGS
	PGCANVAS_INCREMENTAL
	PGCANVAS_MOVEGROP
	PGCANVAS_MUTATEGROP
	PGCANVAS_NUKE
	PGCANVAS_REDRAW
	PGCANVAS_SCROLL
	PGCANVAS_SETGROP
	PGDEFAULT
	PGFONT_ANY
	PGKEY_0
	PGKEY_1
	PGKEY_2
	PGKEY_3
	PGKEY_4
	PGKEY_5
	PGKEY_6
	PGKEY_7
	PGKEY_8
	PGKEY_9
	PGKEY_AMPERSAND
	PGKEY_ASTERISK
	PGKEY_AT
	PGKEY_BACKQUOTE
	PGKEY_BACKSLASH
	PGKEY_BACKSPACE
	PGKEY_BREAK
	PGKEY_CAPSLOCK
	PGKEY_CARET
	PGKEY_CLEAR
	PGKEY_COLON
	PGKEY_COMMA
	PGKEY_DELETE
	PGKEY_DOLLAR
	PGKEY_DOWN
	PGKEY_END
	PGKEY_EQUALS
	PGKEY_ESCAPE
	PGKEY_EURO
	PGKEY_EXCLAIM
	PGKEY_F1
	PGKEY_F10
	PGKEY_F11
	PGKEY_F12
	PGKEY_F13
	PGKEY_F14
	PGKEY_F15
	PGKEY_F2
	PGKEY_F3
	PGKEY_F4
	PGKEY_F5
	PGKEY_F6
	PGKEY_F7
	PGKEY_F8
	PGKEY_F9
	PGKEY_GREATER
	PGKEY_HASH
	PGKEY_HELP
	PGKEY_HOME
	PGKEY_INSERT
	PGKEY_KP0
	PGKEY_KP1
	PGKEY_KP2
	PGKEY_KP3
	PGKEY_KP4
	PGKEY_KP5
	PGKEY_KP6
	PGKEY_KP7
	PGKEY_KP8
	PGKEY_KP9
	PGKEY_KP_DIVIDE
	PGKEY_KP_ENTER
	PGKEY_KP_EQUALS
	PGKEY_KP_MINUS
	PGKEY_KP_MULTIPLY
	PGKEY_KP_PERIOD
	PGKEY_KP_PLUS
	PGKEY_LALT
	PGKEY_LCTRL
	PGKEY_LEFT
	PGKEY_LEFTBRACKET
	PGKEY_LEFTPAREN
	PGKEY_LESS
	PGKEY_LMETA
	PGKEY_LSHIFT
	PGKEY_LSUPER
	PGKEY_MENU
	PGKEY_MINUS
	PGKEY_MODE
	PGKEY_NUMLOCK
	PGKEY_PAGEDOWN
	PGKEY_PAGEUP
	PGKEY_PAUSE
	PGKEY_PERIOD
	PGKEY_PLUS
	PGKEY_POWER
	PGKEY_PRINT
	PGKEY_QUESTION
	PGKEY_QUOTE
	PGKEY_QUOTEDBL
	PGKEY_RALT
	PGKEY_RCTRL
	PGKEY_RETURN
	PGKEY_RIGHT
	PGKEY_RIGHTBRACKET
	PGKEY_RIGHTPAREN
	PGKEY_RMETA
	PGKEY_RSHIFT
	PGKEY_RSUPER
	PGKEY_SCROLLOCK
	PGKEY_SEMICOLON
	PGKEY_SLASH
	PGKEY_SPACE
	PGKEY_SYSREQ
	PGKEY_TAB
	PGKEY_UNDERSCORE
	PGKEY_UP
	PGKEY_WORLD_0
	PGKEY_WORLD_1
	PGKEY_WORLD_10
	PGKEY_WORLD_11
	PGKEY_WORLD_12
	PGKEY_WORLD_13
	PGKEY_WORLD_14
	PGKEY_WORLD_15
	PGKEY_WORLD_16
	PGKEY_WORLD_17
	PGKEY_WORLD_18
	PGKEY_WORLD_19
	PGKEY_WORLD_2
	PGKEY_WORLD_20
	PGKEY_WORLD_21
	PGKEY_WORLD_22
	PGKEY_WORLD_23
	PGKEY_WORLD_24
	PGKEY_WORLD_25
	PGKEY_WORLD_26
	PGKEY_WORLD_27
	PGKEY_WORLD_28
	PGKEY_WORLD_29
	PGKEY_WORLD_3
	PGKEY_WORLD_30
	PGKEY_WORLD_31
	PGKEY_WORLD_32
	PGKEY_WORLD_33
	PGKEY_WORLD_34
	PGKEY_WORLD_35
	PGKEY_WORLD_36
	PGKEY_WORLD_37
	PGKEY_WORLD_38
	PGKEY_WORLD_39
	PGKEY_WORLD_4
	PGKEY_WORLD_40
	PGKEY_WORLD_41
	PGKEY_WORLD_42
	PGKEY_WORLD_43
	PGKEY_WORLD_44
	PGKEY_WORLD_45
	PGKEY_WORLD_46
	PGKEY_WORLD_47
	PGKEY_WORLD_48
	PGKEY_WORLD_49
	PGKEY_WORLD_5
	PGKEY_WORLD_50
	PGKEY_WORLD_51
	PGKEY_WORLD_52
	PGKEY_WORLD_53
	PGKEY_WORLD_54
	PGKEY_WORLD_55
	PGKEY_WORLD_56
	PGKEY_WORLD_57
	PGKEY_WORLD_58
	PGKEY_WORLD_59
	PGKEY_WORLD_6
	PGKEY_WORLD_60
	PGKEY_WORLD_61
	PGKEY_WORLD_62
	PGKEY_WORLD_63
	PGKEY_WORLD_64
	PGKEY_WORLD_65
	PGKEY_WORLD_66
	PGKEY_WORLD_67
	PGKEY_WORLD_68
	PGKEY_WORLD_69
	PGKEY_WORLD_7
	PGKEY_WORLD_70
	PGKEY_WORLD_71
	PGKEY_WORLD_72
	PGKEY_WORLD_73
	PGKEY_WORLD_74
	PGKEY_WORLD_75
	PGKEY_WORLD_76
	PGKEY_WORLD_77
	PGKEY_WORLD_78
	PGKEY_WORLD_79
	PGKEY_WORLD_8
	PGKEY_WORLD_80
	PGKEY_WORLD_81
	PGKEY_WORLD_82
	PGKEY_WORLD_83
	PGKEY_WORLD_84
	PGKEY_WORLD_85
	PGKEY_WORLD_86
	PGKEY_WORLD_87
	PGKEY_WORLD_88
	PGKEY_WORLD_89
	PGKEY_WORLD_9
	PGKEY_WORLD_90
	PGKEY_WORLD_91
	PGKEY_WORLD_92
	PGKEY_WORLD_93
	PGKEY_WORLD_94
	PGKEY_WORLD_95
	PGKEY_a
	PGKEY_b
	PGKEY_c
	PGKEY_d
	PGKEY_e
	PGKEY_f
	PGKEY_g
	PGKEY_h
	PGKEY_i
	PGKEY_j
	PGKEY_k
	PGKEY_l
	PGKEY_m
	PGKEY_n
	PGKEY_o
	PGKEY_p
	PGKEY_q
	PGKEY_r
	PGKEY_s
	PGKEY_t
	PGKEY_u
	PGKEY_v
	PGKEY_w
	PGKEY_x
	PGKEY_y
	PGKEY_z
	PGMEMDAT_NEED_FREE
	PGMEMDAT_NEED_UNMAP
	PGMOD_ALT
	PGMOD_CAPS
	PGMOD_CTRL
	PGMOD_LALT
	PGMOD_LCTRL
	PGMOD_LMETA
	PGMOD_LSHIFT
	PGMOD_META
	PGMOD_MODE
	PGMOD_NUM
	PGMOD_RALT
	PGMOD_RCTRL
	PGMOD_RMETA
	PGMOD_RSHIFT
	PGMOD_SHIFT
	PGREQ_BATCH
	PGREQ_FOCUS
	PGREQ_FREE
	PGREQ_GET
	PGREQ_GETMODE
	PGREQ_GETPAYLOAD
	PGREQ_GETSTRING
	PGREQ_IN_DIRECT
	PGREQ_IN_KEY
	PGREQ_IN_POINT
	PGREQ_MKBITMAP
	PGREQ_MKCONTEXT
	PGREQ_MKFILLSTYLE
	PGREQ_MKFONT
	PGREQ_MKMENU
	PGREQ_MKMSGDLG
	PGREQ_MKPOPUP
	PGREQ_MKSTRING
	PGREQ_MKTHEME
	PGREQ_MKWIDGET
	PGREQ_PING
	PGREQ_REGISTER
	PGREQ_REGOWNER
	PGREQ_RMCONTEXT
	PGREQ_SET
	PGREQ_SETMODE
	PGREQ_SETPAYLOAD
	PGREQ_SIZETEXT
	PGREQ_UNDEF
	PGREQ_UNREGOWNER
	PGREQ_UPDATE
	PGREQ_UPDATEPART
	PGREQ_WAIT
	PGREQ_WRITETO
	PGTH_FORMATVERSION
	PGTH_LOAD_COPY
	PGTH_LOAD_NONE
	PGTH_LOAD_REQUEST
	PGTH_ONUM
	PGTH_OPCMD_AND
	PGTH_OPCMD_COLOR
	PGTH_OPCMD_COLORADD
	PGTH_OPCMD_COLORDIV
	PGTH_OPCMD_COLORMULT
	PGTH_OPCMD_COLORSUB
	PGTH_OPCMD_DIVIDE
	PGTH_OPCMD_EQ
	PGTH_OPCMD_GT
	PGTH_OPCMD_LOCALPROP
	PGTH_OPCMD_LOGICAL_AND
	PGTH_OPCMD_LOGICAL_NOT
	PGTH_OPCMD_LOGICAL_OR
	PGTH_OPCMD_LONGGET
	PGTH_OPCMD_LONGGROP
	PGTH_OPCMD_LONGLITERAL
	PGTH_OPCMD_LONGSET
	PGTH_OPCMD_LT
	PGTH_OPCMD_MINUS
	PGTH_OPCMD_MULTIPLY
	PGTH_OPCMD_OR
	PGTH_OPCMD_PLUS
	PGTH_OPCMD_PROPERTY
	PGTH_OPCMD_QUESTIONCOLON
	PGTH_OPCMD_SHIFTL
	PGTH_OPCMD_SHIFTR
	PGTH_OPSIMPLE_CMDCODE
	PGTH_OPSIMPLE_GET
	PGTH_OPSIMPLE_GROP
	PGTH_OPSIMPLE_LITERAL
	PGTH_OPSIMPLE_SET
	PGTH_O_BACKGROUND
	PGTH_O_BASE_CONTAINER
	PGTH_O_BASE_DISPLAY
	PGTH_O_BASE_INTERACTIVE
	PGTH_O_BASE_PANELBTN
	PGTH_O_BASE_TLCONTAINER
	PGTH_O_BITMAP
	PGTH_O_BOX
	PGTH_O_BUTTON
	PGTH_O_BUTTON_HILIGHT
	PGTH_O_BUTTON_ON
	PGTH_O_CHECKBOX
	PGTH_O_CHECKBOX_HILIGHT
	PGTH_O_CHECKBOX_ON
	PGTH_O_CLOSEBTN
	PGTH_O_CLOSEBTN_HILIGHT
	PGTH_O_CLOSEBTN_ON
	PGTH_O_DEFAULT
	PGTH_O_FIELD
	PGTH_O_FLATBUTTON
	PGTH_O_FLATBUTTON_HILIGHT
	PGTH_O_FLATBUTTON_ON
	PGTH_O_INDICATOR
	PGTH_O_LABEL
	PGTH_O_LABEL_DLGTEXT
	PGTH_O_LABEL_DLGTITLE
	PGTH_O_LABEL_SCROLL
	PGTH_O_MENUITEM
	PGTH_O_MENUITEM_HILIGHT
	PGTH_O_PANEL
	PGTH_O_PANELBAR
	PGTH_O_PANELBAR_HILIGHT
	PGTH_O_PANELBAR_ON
	PGTH_O_POPUP
	PGTH_O_POPUP_MENU
	PGTH_O_POPUP_MESSAGEDLG
	PGTH_O_ROTATEBTN
	PGTH_O_ROTATEBTN_HILIGHT
	PGTH_O_ROTATEBTN_ON
	PGTH_O_SCROLL
	PGTH_O_SCROLL_HILIGHT
	PGTH_O_SCROLL_ON
	PGTH_O_THEMEINFO
	PGTH_O_TOOLBAR
	PGTH_O_ZOOMBTN
	PGTH_O_ZOOMBTN_HILIGHT
	PGTH_O_ZOOMBTN_ON
	PGTH_P_ALIGN
	PGTH_P_BACKDROP
	PGTH_P_BGCOLOR
	PGTH_P_BGFILL
	PGTH_P_BITMAP1
	PGTH_P_BITMAP2
	PGTH_P_BITMAP3
	PGTH_P_BITMAP4
	PGTH_P_BITMAPMARGIN
	PGTH_P_BITMAPSIDE
	PGTH_P_CURSORBITMAP
	PGTH_P_CURSORBITMASK
	PGTH_P_FGCOLOR
	PGTH_P_FONT
	PGTH_P_HEIGHT
	PGTH_P_HILIGHTCOLOR
	PGTH_P_HOTKEY_CANCEL
	PGTH_P_HOTKEY_NO
	PGTH_P_HOTKEY_OK
	PGTH_P_HOTKEY_YES
	PGTH_P_ICON_CANCEL
	PGTH_P_ICON_OK
	PGTH_P_MARGIN
	PGTH_P_NAME
	PGTH_P_OFFSET
	PGTH_P_OVERLAY
	PGTH_P_SHADOWCOLOR
	PGTH_P_SIDE
	PGTH_P_SPACING
	PGTH_P_STRING_CANCEL
	PGTH_P_STRING_NO
	PGTH_P_STRING_OK
	PGTH_P_STRING_YES
	PGTH_P_TEXT
	PGTH_P_WIDGETBITMAP
	PGTH_P_WIDGETBITMASK
	PGTH_P_WIDTH
	PGTH_TAG_AUTHOR
	PGTH_TAG_AUTHOREMAIL
	PGTH_TAG_README
	PGTH_TAG_URL
	PG_AMAX
	PG_APPMAX
	PG_APPSPEC_HEIGHT
	PG_APPSPEC_MAXHEIGHT
	PG_APPSPEC_MAXWIDTH
	PG_APPSPEC_MINHEIGHT
	PG_APPSPEC_MINWIDTH
	PG_APPSPEC_SIDE
	PG_APPSPEC_SIDEMASK
	PG_APPSPEC_WIDTH
	PG_APP_NORMAL
	PG_APP_TOOLBAR
	PG_A_ALL
	PG_A_BOTTOM
	PG_A_CENTER
	PG_A_LEFT
	PG_A_NE
	PG_A_NW
	PG_A_RIGHT
	PG_A_SE
	PG_A_SW
	PG_A_TOP
	PG_DERIVE_AFTER
	PG_DERIVE_BEFORE
	PG_DERIVE_BEFORE_OLD
	PG_DERIVE_INSIDE
	PG_DIR_HORIZONTAL
	PG_DIR_VERTICAL
	PG_ERRT_BADPARAM
	PG_ERRT_BUSY
	PG_ERRT_CLIENT
	PG_ERRT_FILEFMT
	PG_ERRT_HANDLE
	PG_ERRT_INTERNAL
	PG_ERRT_IO
	PG_ERRT_MEMORY
	PG_ERRT_NETWORK
	PG_ERRT_NONE
	PG_EVENTCODINGMASK
	PG_EVENTCODING_DATA
	PG_EVENTCODING_KBD
	PG_EVENTCODING_PARAM
	PG_EVENTCODING_PNTR
	PG_EVENTCODING_XY
	PG_EXEV_CHAR
	PG_EXEV_KEY
	PG_EXEV_NOCLICK
	PG_EXEV_PNTR_DOWN
	PG_EXEV_PNTR_MOVE
	PG_EXEV_PNTR_UP
	PG_EXEV_TOGGLE
	PG_FM_OFF
	PG_FM_ON
	PG_FM_SET
	PG_FM_TOGGLE
	PG_FSTYLE_BOLD
	PG_FSTYLE_DEFAULT
	PG_FSTYLE_DOUBLESPACE
	PG_FSTYLE_DOUBLEWIDTH
	PG_FSTYLE_EXTENDED
	PG_FSTYLE_FIXED
	PG_FSTYLE_FLUSH
	PG_FSTYLE_GRAYLINE
	PG_FSTYLE_IBMEXTEND
	PG_FSTYLE_ITALIC
	PG_FSTYLE_ITALIC2
	PG_FSTYLE_STRIKEOUT
	PG_FSTYLE_SUBSET
	PG_FSTYLE_SYMBOL
	PG_FSTYLE_UNDERLINE
	PG_GROPF_INCREMENTAL
	PG_GROPF_PSEUDOINCREMENTAL
	PG_GROPF_TRANSLATE
	PG_GROP_BAR
	PG_GROP_BITMAP
	PG_GROP_DIM
	PG_GROP_FRAME
	PG_GROP_GRADIENT
	PG_GROP_LINE
	PG_GROP_NULL
	PG_GROP_PIXEL
	PG_GROP_RECT
	PG_GROP_SLAB
	PG_GROP_TEXT
	PG_GROP_TEXTGRID
	PG_GROP_TEXTV
	PG_GROP_TILEBITMAP
	PG_LGOPMAX
	PG_LGOP_AND
	PG_LGOP_INVERT
	PG_LGOP_INVERT_AND
	PG_LGOP_INVERT_OR
	PG_LGOP_INVERT_XOR
	PG_LGOP_NONE
	PG_LGOP_NULL
	PG_LGOP_OR
	PG_LGOP_XOR
	PG_MAX_RESPONSE_SZ
	PG_MSGBTN_CANCEL
	PG_MSGBTN_NO
	PG_MSGBTN_OK
	PG_MSGBTN_YES
	PG_NWE_BGCLICK
	PG_NWE_KBD_CHAR
	PG_NWE_KBD_KEYDOWN
	PG_NWE_KBD_KEYUP
	PG_NWE_PNTR_DOWN
	PG_NWE_PNTR_MOVE
	PG_NWE_PNTR_UP
	PG_OWN_KEYBOARD
	PG_OWN_POINTER
	PG_OWN_SYSEVENTS
	PG_POPUP_ATCURSOR
	PG_POPUP_CENTER
	PG_PROTOCOL_VER
	PG_REQUEST_MAGIC
	PG_REQUEST_PORT
	PG_RESPONSE_DATA
	PG_RESPONSE_ERR
	PG_RESPONSE_EVENT
	PG_RESPONSE_RET
	PG_SZMODEMASK
	PG_SZMODE_CNTFRACT
	PG_SZMODE_PERCENT
	PG_SZMODE_PIXEL
	PG_S_ALL
	PG_S_BOTTOM
	PG_S_LEFT
	PG_S_RIGHT
	PG_S_TOP
	PG_TRIGGER_CHAR
	PG_TRIGGER_DOWN
	PG_TRIGGER_KEYDOWN
	PG_TRIGGER_KEYUP
	PG_TRIGGER_MOVE
	PG_TRIGGER_UP
	PG_TYPE_BITMAP
	PG_TYPE_FILLSTYLE
	PG_TYPE_FONTDESC
	PG_TYPE_STRING
	PG_TYPE_THEME
	PG_TYPE_WIDGET
	PG_VID_DOUBLEBUFFER
	PG_VID_FULLSCREEN
	PG_VID_ROTATE90
	PG_WE_ACTIVATE
	PG_WE_BUILD
	PG_WE_CLOSE
	PG_WE_DATA
	PG_WE_DEACTIVATE
	PG_WE_KBD_CHAR
	PG_WE_KBD_KEYDOWN
	PG_WE_KBD_KEYUP
	PG_WE_PNTR_DOWN
	PG_WE_PNTR_MOVE
	PG_WE_PNTR_UP
	PG_WE_RESIZE
	PG_WIDGETMAX
	PG_WIDGET_BACKGROUND
	PG_WIDGET_BITMAP
	PG_WIDGET_BOX
	PG_WIDGET_BUTTON
	PG_WIDGET_CANVAS
	PG_WIDGET_CHECKBOX
	PG_WIDGET_FIELD
	PG_WIDGET_FLATBUTTON
	PG_WIDGET_INDICATOR
	PG_WIDGET_LABEL
	PG_WIDGET_MENUITEM
	PG_WIDGET_PANEL
	PG_WIDGET_POPUP
	PG_WIDGET_SCROLL
	PG_WIDGET_TERMINAL
	PG_WIDGET_TOOLBAR
	PG_WP_ABSOLUTEX
	PG_WP_ABSOLUTEY
	PG_WP_ALIGN
	PG_WP_BGCOLOR
	PG_WP_BIND
	PG_WP_BITMAP
	PG_WP_BITMASK
	PG_WP_BORDERCOLOR
	PG_WP_COLOR
	PG_WP_DIRECTION
	PG_WP_EXTDEVENTS
	PG_WP_FONT
	PG_WP_HOTKEY
	PG_WP_LGOP
	PG_WP_ON
	PG_WP_SCROLL
	PG_WP_SIDE
	PG_WP_SIZE
	PG_WP_SIZEMODE
	PG_WP_TEXT
	PG_WP_TRANSPARENT
	PG_WP_VALUE
	PG_WP_VIRTUALH
	_H_PG_CANVAS
	_H_PG_CLI_C
	_H_PG_CONSTANTS
	_H_PG_NETWORK
);
our $VERSION = '0.01';

sub AUTOLOAD {
    # This AUTOLOAD is used to 'autoload' constants from the constant()
    # XS function.  If a constant is not found then control is passed
    # to the AUTOLOAD in AutoLoader.

    my $constname;
    our $AUTOLOAD;
    ($constname = $AUTOLOAD) =~ s/.*:://;
    croak "& not defined" if $constname eq 'constant';
    my $val = constant($constname, @_ ? $_[0] : 0);
    if ($! != 0) {
	if ($! =~ /Invalid/ || $!{EINVAL}) {
	    $AutoLoader::AUTOLOAD = $AUTOLOAD;
	    goto &AutoLoader::AUTOLOAD;
	}
	else {
	    croak "Your vendor has not defined PicoGUI macro $constname";
	}
    }
    {
	no strict 'refs';
	# Fixed between 5.005_53 and 5.005_61
	if ($] >= 5.00561) {
	    *$AUTOLOAD = sub () { $val };
	}
	else {
	    *$AUTOLOAD = sub { $val };
	}
    }
    goto &$AUTOLOAD;
}

bootstrap PicoGUI $VERSION;

# Preloaded methods go here.

# Autoload methods go after =cut, and are processed by the autosplit program.

1;
__END__
# Below is stub documentation for your module. You better edit it!

=head1 NAME

PicoGUI - Perl extension for blah blah blah

=head1 SYNOPSIS

  use PicoGUI;
  blah blah blah

=head1 DESCRIPTION

Stub documentation for PicoGUI, created by h2xs. It looks like the
author of the extension was negligent enough to leave the stub
unedited.

Blah blah blah.

=head2 EXPORT

None by default.

=head2 Exportable constants

  NULL
  PGBIND_ANY
  PGCANVAS_COLORCONV
  PGCANVAS_EXECFILL
  PGCANVAS_FINDGROP
  PGCANVAS_GROP
  PGCANVAS_GROPFLAGS
  PGCANVAS_INCREMENTAL
  PGCANVAS_MOVEGROP
  PGCANVAS_MUTATEGROP
  PGCANVAS_NUKE
  PGCANVAS_REDRAW
  PGCANVAS_SCROLL
  PGCANVAS_SETGROP
  PGDEFAULT
  PGFONT_ANY
  PGKEY_0
  PGKEY_1
  PGKEY_2
  PGKEY_3
  PGKEY_4
  PGKEY_5
  PGKEY_6
  PGKEY_7
  PGKEY_8
  PGKEY_9
  PGKEY_AMPERSAND
  PGKEY_ASTERISK
  PGKEY_AT
  PGKEY_BACKQUOTE
  PGKEY_BACKSLASH
  PGKEY_BACKSPACE
  PGKEY_BREAK
  PGKEY_CAPSLOCK
  PGKEY_CARET
  PGKEY_CLEAR
  PGKEY_COLON
  PGKEY_COMMA
  PGKEY_DELETE
  PGKEY_DOLLAR
  PGKEY_DOWN
  PGKEY_END
  PGKEY_EQUALS
  PGKEY_ESCAPE
  PGKEY_EURO
  PGKEY_EXCLAIM
  PGKEY_F1
  PGKEY_F10
  PGKEY_F11
  PGKEY_F12
  PGKEY_F13
  PGKEY_F14
  PGKEY_F15
  PGKEY_F2
  PGKEY_F3
  PGKEY_F4
  PGKEY_F5
  PGKEY_F6
  PGKEY_F7
  PGKEY_F8
  PGKEY_F9
  PGKEY_GREATER
  PGKEY_HASH
  PGKEY_HELP
  PGKEY_HOME
  PGKEY_INSERT
  PGKEY_KP0
  PGKEY_KP1
  PGKEY_KP2
  PGKEY_KP3
  PGKEY_KP4
  PGKEY_KP5
  PGKEY_KP6
  PGKEY_KP7
  PGKEY_KP8
  PGKEY_KP9
  PGKEY_KP_DIVIDE
  PGKEY_KP_ENTER
  PGKEY_KP_EQUALS
  PGKEY_KP_MINUS
  PGKEY_KP_MULTIPLY
  PGKEY_KP_PERIOD
  PGKEY_KP_PLUS
  PGKEY_LALT
  PGKEY_LCTRL
  PGKEY_LEFT
  PGKEY_LEFTBRACKET
  PGKEY_LEFTPAREN
  PGKEY_LESS
  PGKEY_LMETA
  PGKEY_LSHIFT
  PGKEY_LSUPER
  PGKEY_MENU
  PGKEY_MINUS
  PGKEY_MODE
  PGKEY_NUMLOCK
  PGKEY_PAGEDOWN
  PGKEY_PAGEUP
  PGKEY_PAUSE
  PGKEY_PERIOD
  PGKEY_PLUS
  PGKEY_POWER
  PGKEY_PRINT
  PGKEY_QUESTION
  PGKEY_QUOTE
  PGKEY_QUOTEDBL
  PGKEY_RALT
  PGKEY_RCTRL
  PGKEY_RETURN
  PGKEY_RIGHT
  PGKEY_RIGHTBRACKET
  PGKEY_RIGHTPAREN
  PGKEY_RMETA
  PGKEY_RSHIFT
  PGKEY_RSUPER
  PGKEY_SCROLLOCK
  PGKEY_SEMICOLON
  PGKEY_SLASH
  PGKEY_SPACE
  PGKEY_SYSREQ
  PGKEY_TAB
  PGKEY_UNDERSCORE
  PGKEY_UP
  PGKEY_WORLD_0
  PGKEY_WORLD_1
  PGKEY_WORLD_10
  PGKEY_WORLD_11
  PGKEY_WORLD_12
  PGKEY_WORLD_13
  PGKEY_WORLD_14
  PGKEY_WORLD_15
  PGKEY_WORLD_16
  PGKEY_WORLD_17
  PGKEY_WORLD_18
  PGKEY_WORLD_19
  PGKEY_WORLD_2
  PGKEY_WORLD_20
  PGKEY_WORLD_21
  PGKEY_WORLD_22
  PGKEY_WORLD_23
  PGKEY_WORLD_24
  PGKEY_WORLD_25
  PGKEY_WORLD_26
  PGKEY_WORLD_27
  PGKEY_WORLD_28
  PGKEY_WORLD_29
  PGKEY_WORLD_3
  PGKEY_WORLD_30
  PGKEY_WORLD_31
  PGKEY_WORLD_32
  PGKEY_WORLD_33
  PGKEY_WORLD_34
  PGKEY_WORLD_35
  PGKEY_WORLD_36
  PGKEY_WORLD_37
  PGKEY_WORLD_38
  PGKEY_WORLD_39
  PGKEY_WORLD_4
  PGKEY_WORLD_40
  PGKEY_WORLD_41
  PGKEY_WORLD_42
  PGKEY_WORLD_43
  PGKEY_WORLD_44
  PGKEY_WORLD_45
  PGKEY_WORLD_46
  PGKEY_WORLD_47
  PGKEY_WORLD_48
  PGKEY_WORLD_49
  PGKEY_WORLD_5
  PGKEY_WORLD_50
  PGKEY_WORLD_51
  PGKEY_WORLD_52
  PGKEY_WORLD_53
  PGKEY_WORLD_54
  PGKEY_WORLD_55
  PGKEY_WORLD_56
  PGKEY_WORLD_57
  PGKEY_WORLD_58
  PGKEY_WORLD_59
  PGKEY_WORLD_6
  PGKEY_WORLD_60
  PGKEY_WORLD_61
  PGKEY_WORLD_62
  PGKEY_WORLD_63
  PGKEY_WORLD_64
  PGKEY_WORLD_65
  PGKEY_WORLD_66
  PGKEY_WORLD_67
  PGKEY_WORLD_68
  PGKEY_WORLD_69
  PGKEY_WORLD_7
  PGKEY_WORLD_70
  PGKEY_WORLD_71
  PGKEY_WORLD_72
  PGKEY_WORLD_73
  PGKEY_WORLD_74
  PGKEY_WORLD_75
  PGKEY_WORLD_76
  PGKEY_WORLD_77
  PGKEY_WORLD_78
  PGKEY_WORLD_79
  PGKEY_WORLD_8
  PGKEY_WORLD_80
  PGKEY_WORLD_81
  PGKEY_WORLD_82
  PGKEY_WORLD_83
  PGKEY_WORLD_84
  PGKEY_WORLD_85
  PGKEY_WORLD_86
  PGKEY_WORLD_87
  PGKEY_WORLD_88
  PGKEY_WORLD_89
  PGKEY_WORLD_9
  PGKEY_WORLD_90
  PGKEY_WORLD_91
  PGKEY_WORLD_92
  PGKEY_WORLD_93
  PGKEY_WORLD_94
  PGKEY_WORLD_95
  PGKEY_a
  PGKEY_b
  PGKEY_c
  PGKEY_d
  PGKEY_e
  PGKEY_f
  PGKEY_g
  PGKEY_h
  PGKEY_i
  PGKEY_j
  PGKEY_k
  PGKEY_l
  PGKEY_m
  PGKEY_n
  PGKEY_o
  PGKEY_p
  PGKEY_q
  PGKEY_r
  PGKEY_s
  PGKEY_t
  PGKEY_u
  PGKEY_v
  PGKEY_w
  PGKEY_x
  PGKEY_y
  PGKEY_z
  PGMEMDAT_NEED_FREE
  PGMEMDAT_NEED_UNMAP
  PGMOD_ALT
  PGMOD_CAPS
  PGMOD_CTRL
  PGMOD_LALT
  PGMOD_LCTRL
  PGMOD_LMETA
  PGMOD_LSHIFT
  PGMOD_META
  PGMOD_MODE
  PGMOD_NUM
  PGMOD_RALT
  PGMOD_RCTRL
  PGMOD_RMETA
  PGMOD_RSHIFT
  PGMOD_SHIFT
  PGREQ_BATCH
  PGREQ_FOCUS
  PGREQ_FREE
  PGREQ_GET
  PGREQ_GETMODE
  PGREQ_GETPAYLOAD
  PGREQ_GETSTRING
  PGREQ_IN_DIRECT
  PGREQ_IN_KEY
  PGREQ_IN_POINT
  PGREQ_MKBITMAP
  PGREQ_MKCONTEXT
  PGREQ_MKFILLSTYLE
  PGREQ_MKFONT
  PGREQ_MKMENU
  PGREQ_MKMSGDLG
  PGREQ_MKPOPUP
  PGREQ_MKSTRING
  PGREQ_MKTHEME
  PGREQ_MKWIDGET
  PGREQ_PING
  PGREQ_REGISTER
  PGREQ_REGOWNER
  PGREQ_RMCONTEXT
  PGREQ_SET
  PGREQ_SETMODE
  PGREQ_SETPAYLOAD
  PGREQ_SIZETEXT
  PGREQ_UNDEF
  PGREQ_UNREGOWNER
  PGREQ_UPDATE
  PGREQ_UPDATEPART
  PGREQ_WAIT
  PGREQ_WRITETO
  PGTH_FORMATVERSION
  PGTH_LOAD_COPY
  PGTH_LOAD_NONE
  PGTH_LOAD_REQUEST
  PGTH_ONUM
  PGTH_OPCMD_AND
  PGTH_OPCMD_COLOR
  PGTH_OPCMD_COLORADD
  PGTH_OPCMD_COLORDIV
  PGTH_OPCMD_COLORMULT
  PGTH_OPCMD_COLORSUB
  PGTH_OPCMD_DIVIDE
  PGTH_OPCMD_EQ
  PGTH_OPCMD_GT
  PGTH_OPCMD_LOCALPROP
  PGTH_OPCMD_LOGICAL_AND
  PGTH_OPCMD_LOGICAL_NOT
  PGTH_OPCMD_LOGICAL_OR
  PGTH_OPCMD_LONGGET
  PGTH_OPCMD_LONGGROP
  PGTH_OPCMD_LONGLITERAL
  PGTH_OPCMD_LONGSET
  PGTH_OPCMD_LT
  PGTH_OPCMD_MINUS
  PGTH_OPCMD_MULTIPLY
  PGTH_OPCMD_OR
  PGTH_OPCMD_PLUS
  PGTH_OPCMD_PROPERTY
  PGTH_OPCMD_QUESTIONCOLON
  PGTH_OPCMD_SHIFTL
  PGTH_OPCMD_SHIFTR
  PGTH_OPSIMPLE_CMDCODE
  PGTH_OPSIMPLE_GET
  PGTH_OPSIMPLE_GROP
  PGTH_OPSIMPLE_LITERAL
  PGTH_OPSIMPLE_SET
  PGTH_O_BACKGROUND
  PGTH_O_BASE_CONTAINER
  PGTH_O_BASE_DISPLAY
  PGTH_O_BASE_INTERACTIVE
  PGTH_O_BASE_PANELBTN
  PGTH_O_BASE_TLCONTAINER
  PGTH_O_BITMAP
  PGTH_O_BOX
  PGTH_O_BUTTON
  PGTH_O_BUTTON_HILIGHT
  PGTH_O_BUTTON_ON
  PGTH_O_CHECKBOX
  PGTH_O_CHECKBOX_HILIGHT
  PGTH_O_CHECKBOX_ON
  PGTH_O_CLOSEBTN
  PGTH_O_CLOSEBTN_HILIGHT
  PGTH_O_CLOSEBTN_ON
  PGTH_O_DEFAULT
  PGTH_O_FIELD
  PGTH_O_FLATBUTTON
  PGTH_O_FLATBUTTON_HILIGHT
  PGTH_O_FLATBUTTON_ON
  PGTH_O_INDICATOR
  PGTH_O_LABEL
  PGTH_O_LABEL_DLGTEXT
  PGTH_O_LABEL_DLGTITLE
  PGTH_O_LABEL_SCROLL
  PGTH_O_MENUITEM
  PGTH_O_MENUITEM_HILIGHT
  PGTH_O_PANEL
  PGTH_O_PANELBAR
  PGTH_O_PANELBAR_HILIGHT
  PGTH_O_PANELBAR_ON
  PGTH_O_POPUP
  PGTH_O_POPUP_MENU
  PGTH_O_POPUP_MESSAGEDLG
  PGTH_O_ROTATEBTN
  PGTH_O_ROTATEBTN_HILIGHT
  PGTH_O_ROTATEBTN_ON
  PGTH_O_SCROLL
  PGTH_O_SCROLL_HILIGHT
  PGTH_O_SCROLL_ON
  PGTH_O_THEMEINFO
  PGTH_O_TOOLBAR
  PGTH_O_ZOOMBTN
  PGTH_O_ZOOMBTN_HILIGHT
  PGTH_O_ZOOMBTN_ON
  PGTH_P_ALIGN
  PGTH_P_BACKDROP
  PGTH_P_BGCOLOR
  PGTH_P_BGFILL
  PGTH_P_BITMAP1
  PGTH_P_BITMAP2
  PGTH_P_BITMAP3
  PGTH_P_BITMAP4
  PGTH_P_BITMAPMARGIN
  PGTH_P_BITMAPSIDE
  PGTH_P_CURSORBITMAP
  PGTH_P_CURSORBITMASK
  PGTH_P_FGCOLOR
  PGTH_P_FONT
  PGTH_P_HEIGHT
  PGTH_P_HILIGHTCOLOR
  PGTH_P_HOTKEY_CANCEL
  PGTH_P_HOTKEY_NO
  PGTH_P_HOTKEY_OK
  PGTH_P_HOTKEY_YES
  PGTH_P_ICON_CANCEL
  PGTH_P_ICON_OK
  PGTH_P_MARGIN
  PGTH_P_NAME
  PGTH_P_OFFSET
  PGTH_P_OVERLAY
  PGTH_P_SHADOWCOLOR
  PGTH_P_SIDE
  PGTH_P_SPACING
  PGTH_P_STRING_CANCEL
  PGTH_P_STRING_NO
  PGTH_P_STRING_OK
  PGTH_P_STRING_YES
  PGTH_P_TEXT
  PGTH_P_WIDGETBITMAP
  PGTH_P_WIDGETBITMASK
  PGTH_P_WIDTH
  PGTH_TAG_AUTHOR
  PGTH_TAG_AUTHOREMAIL
  PGTH_TAG_README
  PGTH_TAG_URL
  PG_AMAX
  PG_APPMAX
  PG_APPSPEC_HEIGHT
  PG_APPSPEC_MAXHEIGHT
  PG_APPSPEC_MAXWIDTH
  PG_APPSPEC_MINHEIGHT
  PG_APPSPEC_MINWIDTH
  PG_APPSPEC_SIDE
  PG_APPSPEC_SIDEMASK
  PG_APPSPEC_WIDTH
  PG_APP_NORMAL
  PG_APP_TOOLBAR
  PG_A_ALL
  PG_A_BOTTOM
  PG_A_CENTER
  PG_A_LEFT
  PG_A_NE
  PG_A_NW
  PG_A_RIGHT
  PG_A_SE
  PG_A_SW
  PG_A_TOP
  PG_DERIVE_AFTER
  PG_DERIVE_BEFORE
  PG_DERIVE_BEFORE_OLD
  PG_DERIVE_INSIDE
  PG_DIR_HORIZONTAL
  PG_DIR_VERTICAL
  PG_ERRT_BADPARAM
  PG_ERRT_BUSY
  PG_ERRT_CLIENT
  PG_ERRT_FILEFMT
  PG_ERRT_HANDLE
  PG_ERRT_INTERNAL
  PG_ERRT_IO
  PG_ERRT_MEMORY
  PG_ERRT_NETWORK
  PG_ERRT_NONE
  PG_EVENTCODINGMASK
  PG_EVENTCODING_DATA
  PG_EVENTCODING_KBD
  PG_EVENTCODING_PARAM
  PG_EVENTCODING_PNTR
  PG_EVENTCODING_XY
  PG_EXEV_CHAR
  PG_EXEV_KEY
  PG_EXEV_NOCLICK
  PG_EXEV_PNTR_DOWN
  PG_EXEV_PNTR_MOVE
  PG_EXEV_PNTR_UP
  PG_EXEV_TOGGLE
  PG_FM_OFF
  PG_FM_ON
  PG_FM_SET
  PG_FM_TOGGLE
  PG_FSTYLE_BOLD
  PG_FSTYLE_DEFAULT
  PG_FSTYLE_DOUBLESPACE
  PG_FSTYLE_DOUBLEWIDTH
  PG_FSTYLE_EXTENDED
  PG_FSTYLE_FIXED
  PG_FSTYLE_FLUSH
  PG_FSTYLE_GRAYLINE
  PG_FSTYLE_IBMEXTEND
  PG_FSTYLE_ITALIC
  PG_FSTYLE_ITALIC2
  PG_FSTYLE_STRIKEOUT
  PG_FSTYLE_SUBSET
  PG_FSTYLE_SYMBOL
  PG_FSTYLE_UNDERLINE
  PG_GROPF_INCREMENTAL
  PG_GROPF_PSEUDOINCREMENTAL
  PG_GROPF_TRANSLATE
  PG_GROP_BAR
  PG_GROP_BITMAP
  PG_GROP_DIM
  PG_GROP_FRAME
  PG_GROP_GRADIENT
  PG_GROP_LINE
  PG_GROP_NULL
  PG_GROP_PIXEL
  PG_GROP_RECT
  PG_GROP_SLAB
  PG_GROP_TEXT
  PG_GROP_TEXTGRID
  PG_GROP_TEXTV
  PG_GROP_TILEBITMAP
  PG_LGOPMAX
  PG_LGOP_AND
  PG_LGOP_INVERT
  PG_LGOP_INVERT_AND
  PG_LGOP_INVERT_OR
  PG_LGOP_INVERT_XOR
  PG_LGOP_NONE
  PG_LGOP_NULL
  PG_LGOP_OR
  PG_LGOP_XOR
  PG_MAX_RESPONSE_SZ
  PG_MSGBTN_CANCEL
  PG_MSGBTN_NO
  PG_MSGBTN_OK
  PG_MSGBTN_YES
  PG_NWE_BGCLICK
  PG_NWE_KBD_CHAR
  PG_NWE_KBD_KEYDOWN
  PG_NWE_KBD_KEYUP
  PG_NWE_PNTR_DOWN
  PG_NWE_PNTR_MOVE
  PG_NWE_PNTR_UP
  PG_OWN_KEYBOARD
  PG_OWN_POINTER
  PG_OWN_SYSEVENTS
  PG_POPUP_ATCURSOR
  PG_POPUP_CENTER
  PG_PROTOCOL_VER
  PG_REQUEST_MAGIC
  PG_REQUEST_PORT
  PG_RESPONSE_DATA
  PG_RESPONSE_ERR
  PG_RESPONSE_EVENT
  PG_RESPONSE_RET
  PG_SZMODEMASK
  PG_SZMODE_CNTFRACT
  PG_SZMODE_PERCENT
  PG_SZMODE_PIXEL
  PG_S_ALL
  PG_S_BOTTOM
  PG_S_LEFT
  PG_S_RIGHT
  PG_S_TOP
  PG_TRIGGER_CHAR
  PG_TRIGGER_DOWN
  PG_TRIGGER_KEYDOWN
  PG_TRIGGER_KEYUP
  PG_TRIGGER_MOVE
  PG_TRIGGER_UP
  PG_TYPE_BITMAP
  PG_TYPE_FILLSTYLE
  PG_TYPE_FONTDESC
  PG_TYPE_STRING
  PG_TYPE_THEME
  PG_TYPE_WIDGET
  PG_VID_DOUBLEBUFFER
  PG_VID_FULLSCREEN
  PG_VID_ROTATE90
  PG_WE_ACTIVATE
  PG_WE_BUILD
  PG_WE_CLOSE
  PG_WE_DATA
  PG_WE_DEACTIVATE
  PG_WE_KBD_CHAR
  PG_WE_KBD_KEYDOWN
  PG_WE_KBD_KEYUP
  PG_WE_PNTR_DOWN
  PG_WE_PNTR_MOVE
  PG_WE_PNTR_UP
  PG_WE_RESIZE
  PG_WIDGETMAX
  PG_WIDGET_BACKGROUND
  PG_WIDGET_BITMAP
  PG_WIDGET_BOX
  PG_WIDGET_BUTTON
  PG_WIDGET_CANVAS
  PG_WIDGET_CHECKBOX
  PG_WIDGET_FIELD
  PG_WIDGET_FLATBUTTON
  PG_WIDGET_INDICATOR
  PG_WIDGET_LABEL
  PG_WIDGET_MENUITEM
  PG_WIDGET_PANEL
  PG_WIDGET_POPUP
  PG_WIDGET_SCROLL
  PG_WIDGET_TERMINAL
  PG_WIDGET_TOOLBAR
  PG_WP_ABSOLUTEX
  PG_WP_ABSOLUTEY
  PG_WP_ALIGN
  PG_WP_BGCOLOR
  PG_WP_BIND
  PG_WP_BITMAP
  PG_WP_BITMASK
  PG_WP_BORDERCOLOR
  PG_WP_COLOR
  PG_WP_DIRECTION
  PG_WP_EXTDEVENTS
  PG_WP_FONT
  PG_WP_HOTKEY
  PG_WP_LGOP
  PG_WP_ON
  PG_WP_SCROLL
  PG_WP_SIDE
  PG_WP_SIZE
  PG_WP_SIZEMODE
  PG_WP_TEXT
  PG_WP_TRANSPARENT
  PG_WP_VALUE
  PG_WP_VIRTUALH
  _H_PG_CANVAS
  _H_PG_CLI_C
  _H_PG_CONSTANTS
  _H_PG_NETWORK


=head1 AUTHOR

A. U. Thor, a.u.thor@a.galaxy.far.far.away

=head1 SEE ALSO

perl(1).

=cut
