-- Generated from constants.h network.h pgkeys.h canvas.h
-- Do not manually update
-- Shorten pg_widget to pg_w, pg_trigger to pg_t
-- convert eg pgc_red to pico.usualcolor.red 
------------------------------------------------------------
Pico = {} 
Pico.handler={}  -- event handler 
Pico.usualcolor = {}
Pico.app_normal                     = 1
Pico.app_toolbar                    = 2
Pico.appmax                         = 2
Pico.appspec_side                   = 1
Pico.appspec_sidemask               = 2
Pico.appspec_width                  = 3
Pico.appspec_height                 = 4
Pico.appspec_minwidth               = 5
Pico.appspec_maxwidth               = 6
Pico.appspec_minheight              = 7
Pico.appspec_maxheight              = 8
Pico.own_display                    = 4
Pico.a_center                       = 0
Pico.a_top                          = 1
Pico.a_left                         = 2
Pico.a_bottom                       = 3
Pico.a_right                        = 4
Pico.a_nw                           = 5
Pico.a_sw                           = 6
Pico.a_ne                           = 7
Pico.a_se                           = 8
Pico.a_all                          = 9
Pico.amax                           = 9
Pico.s_top                          = 8
Pico.s_bottom                       = 16
Pico.s_left                         = 32
Pico.s_right                        = 64
Pico.s_all                          = 2048
Pico.fstyle_fixed                   = 1
Pico.fstyle_default                 = 2
Pico.fstyle_symbol                  = 4
Pico.fstyle_subset                  = 8
Pico.fstyle_encoding_isolatin1      = 16
Pico.fstyle_encoding_ibm            = 32
Pico.fstyle_doublespace             = 128
Pico.fstyle_bold                    = 256
Pico.fstyle_italic                  = 512
Pico.fstyle_underline               = 1024
Pico.fstyle_strikeout               = 2048
Pico.fstyle_grayline                = 4096
Pico.fstyle_flush                   = 16384
Pico.fstyle_doublewidth             = 32768
Pico.fstyle_italic2                 = 65536
Pico.fstyle_encoding_unicode        = 131072
Pico.fstyle_condensed               = 262144
Pico.fr_bitmap_normal               = 1
Pico.fr_bitmap_bold                 = 2
Pico.fr_bitmap_italic               = 4
Pico.fr_bitmap_bolditalic           = 8
Pico.fr_scalable                    = 16
Pico.errt_none                      = 0
Pico.errt_memory                    = 256
Pico.errt_io                        = 512
Pico.errt_network                   = 768
Pico.errt_badparam                  = 1024
Pico.errt_handle                    = 1280
Pico.errt_internal                  = 1536
Pico.errt_busy                      = 1792
Pico.errt_filefmt                   = 2048
Pico.errt_client                    = 32768
Pico.type_bitmap                    = 1
Pico.type_widget                    = 2
Pico.type_fontdesc                  = 3
Pico.type_pgstring                  = 4
Pico.type_theme                     = 5
Pico.type_fillstyle                 = 6
Pico.type_array                     = 7
Pico.type_driver                    = 8
Pico.type_palette                   = 9
Pico.type_wt                        = 11
Pico.type_infilter                  = 12
Pico.type_cursor                    = 13
Pico.type_paragraph                 = 14
Pico.typemask                       = 31
Pico.th_o_default                   = 0
Pico.th_o_base_interactive          = 1
Pico.th_o_base_container            = 2
Pico.th_o_button                    = 3
Pico.th_o_button_hilight            = 4
Pico.th_o_button_on                 = 5
Pico.th_o_toolbar                   = 6
Pico.th_o_scroll                    = 7
Pico.th_o_scroll_hilight            = 8
Pico.th_o_indicator                 = 9
Pico.th_o_panel                     = 10
Pico.th_o_panelbar                  = 11
Pico.th_o_popup                     = 12
Pico.th_o_background                = 13
Pico.th_o_base_display              = 14
Pico.th_o_base_tlcontainer          = 15
Pico.th_o_themeinfo                 = 16
Pico.th_o_label                     = 17
Pico.th_o_field                     = 18
Pico.th_o_bitmap                    = 19
Pico.th_o_scroll_on                 = 20
Pico.th_o_label_scroll              = 21
Pico.th_o_panelbar_hilight          = 22
Pico.th_o_panelbar_on               = 23
Pico.th_o_box                       = 24
Pico.th_o_label_dlgtitle            = 25
Pico.th_o_label_dlgtext             = 26
Pico.th_o_closebtn                  = 27
Pico.th_o_closebtn_on               = 28
Pico.th_o_closebtn_hilight          = 29
Pico.th_o_base_panelbtn             = 30
Pico.th_o_rotatebtn                 = 31
Pico.th_o_rotatebtn_on              = 32
Pico.th_o_rotatebtn_hilight         = 33
Pico.th_o_zoombtn                   = 34
Pico.th_o_zoombtn_on                = 35
Pico.th_o_zoombtn_hilight           = 36
Pico.th_o_popup_menu                = 37
Pico.th_o_popup_messagedlg          = 38
Pico.th_o_menuitem                  = 39
Pico.th_o_menuitem_hilight          = 40
Pico.th_o_checkbox                  = 41
Pico.th_o_checkbox_hilight          = 42
Pico.th_o_checkbox_on               = 43
Pico.th_o_flatbutton                = 44
Pico.th_o_flatbutton_hilight        = 45
Pico.th_o_flatbutton_on             = 46
Pico.th_o_listitem                  = 47
Pico.th_o_listitem_hilight          = 48
Pico.th_o_listitem_on               = 49
Pico.th_o_checkbox_on_nohilight     = 50
Pico.th_o_submenuitem               = 51
Pico.th_o_submenuitem_hilight       = 52
Pico.th_o_radiobutton               = 53
Pico.th_o_radiobutton_hilight       = 54
Pico.th_o_radiobutton_on            = 55
Pico.th_o_radiobutton_on_nohilight  = 56
Pico.th_o_textbox                   = 57
Pico.th_o_terminal                  = 58
Pico.th_o_menubutton                = 60
Pico.th_o_menubutton_on             = 61
Pico.th_o_menubutton_hilight        = 62
Pico.th_o_label_hilight             = 63
Pico.th_o_box_hilight               = 64
Pico.th_o_indicator_h               = 65
Pico.th_o_indicator_v               = 66
Pico.th_o_scroll_h                  = 67
Pico.th_o_scroll_v                  = 68
Pico.th_o_scroll_h_on               = 69
Pico.th_o_scroll_h_hilight          = 70
Pico.th_o_scroll_v_on               = 71
Pico.th_o_scroll_v_hilight          = 72
Pico.th_o_panelbar_h                = 73
Pico.th_o_panelbar_v                = 74
Pico.th_o_panelbar_h_on             = 75
Pico.th_o_panelbar_h_hilight        = 76
Pico.th_o_panelbar_v_on             = 77
Pico.th_o_panelbar_v_hilight        = 78
Pico.th_o_textedit                  = 79
Pico.th_o_managedwindow             = 80
Pico.th_onum                        = 81
Pico.th_o_custom                    = 32767
Pico.th_load_none                   = 0
Pico.th_load_request                = 1
Pico.th_load_copy                   = 2
Pico.th_load_findthobj              = 3
Pico.th_p_bgcolor                   = 1
Pico.th_p_fgcolor                   = 2
Pico.th_p_bgfill                    = 3
Pico.th_p_overlay                   = 4
Pico.th_p_font                      = 5
Pico.th_p_name                      = 6
Pico.th_p_width                     = 7
Pico.th_p_height                    = 8
Pico.th_p_margin                    = 9
Pico.th_p_hilightcolor              = 10
Pico.th_p_shadowcolor               = 11
Pico.th_p_offset                    = 12
Pico.th_p_align                     = 13
Pico.th_p_bitmapside                = 14
Pico.th_p_bitmapmargin              = 15
Pico.th_p_bitmap1                   = 16
Pico.th_p_bitmap2                   = 17
Pico.th_p_bitmap3                   = 18
Pico.th_p_bitmap4                   = 19
Pico.th_p_spacing                   = 20
Pico.th_p_text                      = 21
Pico.th_p_side                      = 22
Pico.th_p_backdrop                  = 23
Pico.th_p_widgetbitmap              = 24
Pico.th_p_widgetbitmask             = 25
Pico.th_p_cursorbitmap              = 26
Pico.th_p_cursorbitmask             = 27
Pico.th_p_hidehotkeys               = 28
Pico.th_p_attr_default              = 29
Pico.th_p_attr_cursor               = 30
Pico.th_p_textcolors                = 31
Pico.th_p_time_on                   = 32
Pico.th_p_time_off                  = 33
Pico.th_p_time_delay                = 34
Pico.th_p_parent                    = 35
Pico.th_p_xoffset                   = 36
Pico.th_p_yoffset                   = 37
Pico.th_p_ticks                     = 38
Pico.th_p_crsrhotspot_x             = 39
Pico.th_p_crsrhotspot_y             = 40
Pico.th_p_cursor_width              = 41
Pico.th_p_border_size               = 42
Pico.th_p_border_fill               = 43
Pico.th_p_object_on                 = 60
Pico.th_p_object_on_nohilight       = 61
Pico.th_p_object_hilight            = 62
Pico.th_p_icon_ok                   = 1000
Pico.th_p_icon_ok_mask              = 1001
Pico.th_p_icon_cancel               = 1002
Pico.th_p_icon_cancel_mask          = 1003
Pico.th_p_icon_yes                  = 1004
Pico.th_p_icon_yes_mask             = 1005
Pico.th_p_icon_no                   = 1006
Pico.th_p_icon_no_mask              = 1007
Pico.th_p_icon_error                = 1008
Pico.th_p_icon_error_mask           = 1009
Pico.th_p_icon_message              = 1010
Pico.th_p_icon_message_mask         = 1011
Pico.th_p_icon_question             = 1012
Pico.th_p_icon_question_mask        = 1013
Pico.th_p_icon_warning              = 1014
Pico.th_p_icon_warning_mask         = 1015
Pico.th_p_hotkey_ok                 = 1501
Pico.th_p_hotkey_cancel             = 1502
Pico.th_p_hotkey_yes                = 1503
Pico.th_p_hotkey_no                 = 1504
Pico.th_p_hotkey_up                 = 1505
Pico.th_p_hotkey_down               = 1506
Pico.th_p_hotkey_left               = 1507
Pico.th_p_hotkey_right              = 1508
Pico.th_p_hotkey_activate           = 1509
Pico.th_p_hotkey_next               = 1510
Pico.th_p_user                      = 10000
Pico.th_p_themeauto                 = 20000
Pico.hhk_none                       = 0
Pico.hhk_return_escape              = 1
Pico.th_tag_author                  = 1
Pico.th_tag_authoremail             = 2
Pico.th_tag_url                     = 3
Pico.th_tag_readme                  = 4
Pico.th_opsimple_grop               = 128
Pico.th_opsimple_literal            = 64
Pico.th_opsimple_cmdcode            = 32
Pico.th_opsimple_get                = 16
Pico.th_opsimple_set                = 0
Pico.th_opcmd_longliteral           = 32
Pico.th_opcmd_plus                  = 33
Pico.th_opcmd_minus                 = 34
Pico.th_opcmd_multiply              = 35
Pico.th_opcmd_divide                = 36
Pico.th_opcmd_shiftl                = 37
Pico.th_opcmd_shiftr                = 38
Pico.th_opcmd_or                    = 39
Pico.th_opcmd_and                   = 40
Pico.th_opcmd_longgrop              = 41
Pico.th_opcmd_longget               = 42
Pico.th_opcmd_longset               = 43
Pico.th_opcmd_property              = 44
Pico.th_opcmd_localprop             = 45
Pico.th_opcmd_coloradd              = 47
Pico.th_opcmd_colorsub              = 48
Pico.th_opcmd_colormult             = 49
Pico.th_opcmd_colordiv              = 50
Pico.th_opcmd_questioncolon         = 51
Pico.th_opcmd_eq                    = 52
Pico.th_opcmd_lt                    = 53
Pico.th_opcmd_gt                    = 54
Pico.th_opcmd_logical_or            = 55
Pico.th_opcmd_logical_and           = 56
Pico.th_opcmd_logical_not           = 57
Pico.th_opcmd_widget                = 58
Pico.th_opcmd_traversewgt           = 59
Pico.th_opcmd_getwidget             = 60
Pico.th_opcmd_call                  = 61
Pico.th_opcmd_localcall             = 62
Pico.grop_rect                      = 0
Pico.grop_frame                     = 16
Pico.grop_slab                      = 32
Pico.grop_bar                       = 48
Pico.grop_pixel                     = 64
Pico.grop_line                      = 80
Pico.grop_ellipse                   = 96
Pico.grop_fellipse                  = 112
Pico.grop_text                      = 4
Pico.grop_bitmap                    = 20
Pico.grop_tilebitmap                = 36
Pico.grop_fpolygon                  = 52
Pico.grop_blur                      = 68
Pico.grop_paragraph                 = 84
Pico.grop_paragraph_inc             = 100
Pico.grop_rotatebitmap              = 116
Pico.grop_textrect                  = 132
Pico.grop_gradient                  = 12
Pico.grop_textgrid                  = 28
Pico.grop_nop                       = 3
Pico.grop_resetclip                 = 19
Pico.grop_setoffset                 = 1
Pico.grop_setclip                   = 17
Pico.grop_setsrc                    = 33
Pico.grop_setmapping                = 5
Pico.grop_setcolor                  = 7
Pico.grop_setfont                   = 23
Pico.grop_setlgop                   = 39
Pico.grop_setangle                  = 55
Pico.grop_vidupdate                 = 2048
Pico.grop_user                      = 4096
Pico.gropf_translate                = 1
Pico.gropf_incremental              = 2
Pico.gropf_pseudoincremental        = 4
Pico.gropf_transient                = 8
Pico.gropf_colored                  = 16
Pico.gropf_universal                = 32
Pico.map_none                       = 0
Pico.map_scale                      = 1
Pico.map_squarescale                = 2
Pico.map_center                     = 3
Pico.lgop_null                      = 0
Pico.lgop_none                      = 1
Pico.lgop_or                        = 2
Pico.lgop_and                       = 3
Pico.lgop_xor                       = 4
Pico.lgop_invert                    = 5
Pico.lgop_invert_or                 = 6
Pico.lgop_invert_and                = 7
Pico.lgop_invert_xor                = 8
Pico.lgop_add                       = 9
Pico.lgop_subtract                  = 10
Pico.lgop_multiply                  = 11
Pico.lgop_stipple                   = 12
Pico.lgop_alpha                     = 13
Pico.lgopmax                        = 13
Pico.vid_fullscreen                 = 1
Pico.vid_doublebuffer               = 2
Pico.vid_rootless                   = 256
Pico.vid_rotate90                   = 4
Pico.vid_rotate180                  = 8
Pico.vid_rotate270                  = 16
Pico.vid_rotatemask                 = 28
Pico.vid_rotbase90                  = 32
Pico.vid_rotbase180                 = 64
Pico.vid_rotbase270                 = 128
Pico.vid_rotbasemask                = 224
Pico.fm_set                         = 0
Pico.fm_on                          = 1
Pico.fm_off                         = 2
Pico.fm_toggle                      = 3
Pico.bitformat_rotate90             = 1
Pico.bitformat_rotate180            = 2
Pico.bitformat_rotate270            = 4
Pico.bitformat_grayscale            = 8
Pico.bitformat_indexed              = 16
Pico.bitformat_symbolic             = 32
Pico.bitformat_truecolor            = 64
Pico.bitformat_alpha                = 128
Pico.dm_backlight                   = 2
Pico.dm_soundfx                     = 3
Pico.dm_power                       = 4
Pico.dm_sdc_char                    = 5
Pico.dm_brightness                  = 6
Pico.dm_contrast                    = 7
Pico.dm_signal                      = 13
Pico.dm_ready                       = 14
Pico.snd_keyclick                   = 1
Pico.snd_beep                       = 2
Pico.snd_visualbell                 = 3
Pico.snd_alarm                      = 4
Pico.snd_shortbeep                  = 5
Pico.power_off                      = 0
Pico.power_sleep                    = 50
Pico.power_vidblank                 = 70
Pico.power_full                     = 100
Pico.derive_before_old              = 0
Pico.derive_after                   = 1
Pico.derive_inside                  = 2
Pico.derive_before                  = 3
Pico.traverse_children              = 1
Pico.traverse_forward               = 2
Pico.traverse_backward              = 3
Pico.traverse_container             = 4
Pico.traverse_app                   = 5
Pico.w_toolbar                      = 0
Pico.w_label                        = 1
Pico.w_scroll                       = 2
Pico.w_indicator                    = 3
Pico.w_managedwindow                = 4
Pico.w_button                       = 5
Pico.w_panel                        = 6
Pico.w_popup                        = 7
Pico.w_box                          = 8
Pico.w_field                        = 9
Pico.w_background                   = 10
Pico.w_menuitem                     = 11
Pico.w_terminal                     = 12
Pico.w_canvas                       = 13
Pico.w_checkbox                     = 14
Pico.w_flatbutton                   = 15
Pico.w_listitem                     = 16
Pico.w_submenuitem                  = 17
Pico.w_radiobutton                  = 18
Pico.w_textbox                      = 19
Pico.w_panelbar                     = 20
Pico.w_simplemenu                   = 21
Pico.w_dialogbox                    = 22
Pico.w_messagedialog                = 23
Pico.w_scrollbox                    = 24
Pico.w_textedit                     = 25
Pico.wmax                           = 25
Pico.wp_size                        = 1
Pico.wp_side                        = 2
Pico.wp_align                       = 3
Pico.wp_bgcolor                     = 4
Pico.wp_color                       = 5
Pico.wp_sizemode                    = 6
Pico.wp_text                        = 7
Pico.wp_font                        = 8
Pico.wp_transparent                 = 9
Pico.wp_bordercolor                 = 10
Pico.wp_bitmap                      = 12
Pico.wp_lgop                        = 13
Pico.wp_value                       = 14
Pico.wp_bitmask                     = 15
Pico.wp_bind                        = 16
Pico.wp_scroll_x                    = 17
Pico.wp_scroll_y                    = 18
Pico.wp_hotkey                      = 19
Pico.wp_extdevents                  = 20
Pico.wp_direction                   = 21
Pico.wp_absolutex                   = 22
Pico.wp_absolutey                   = 23
Pico.wp_on                          = 24
Pico.wp_state                       = 25
Pico.wp_thobj                       = 25
Pico.wp_name                        = 26
Pico.wp_publicbox                   = 27
Pico.wp_disabled                    = 28
Pico.wp_margin                      = 29
Pico.wp_textformat                  = 30
Pico.wp_triggermask                 = 31
Pico.wp_hilighted                   = 32
Pico.wp_selected                    = 33
Pico.wp_selected_handle             = 34
Pico.wp_autoscroll                  = 35
Pico.wp_lines                       = 36
Pico.wp_preferred_w                 = 37
Pico.wp_preferred_h                 = 38
Pico.wp_panelbar                    = 39
Pico.wp_auto_orientation            = 40
Pico.wp_thobj_button                = 41
Pico.wp_thobj_button_hilight        = 42
Pico.wp_thobj_button_on             = 43
Pico.wp_thobj_button_on_nohilight   = 44
Pico.wp_panelbar_label              = 45
Pico.wp_panelbar_close              = 46
Pico.wp_panelbar_rotate             = 47
Pico.wp_panelbar_zoom               = 48
Pico.wp_bitmapside                  = 49
Pico.wp_password                    = 50
Pico.wp_hotkey_flags                = 51
Pico.wp_hotkey_consume              = 52
Pico.wp_width                       = 53
Pico.wp_height                      = 54
Pico.wp_spacing                     = 55
Pico.wp_minimum                     = 56
Pico.wp_multiline                   = 57
Pico.wp_selection                   = 58
Pico.wp_readonly                    = 59
Pico.wp_insertmode                  = 60
Pico.wp_type                        = 61
Pico.szmode_pixel                   = 0
Pico.szmode_percent                 = 4
Pico.szmode_cntfract                = 32768
Pico.insert_overwrite               = 0
Pico.insert_append                  = 1
Pico.insert_prepend                 = 2
Pico.insert_atcursor                = 3
Pico.insertmax                      = 3
Pico.popup_center                   = -1
Pico.popup_atcursor                 = -2
Pico.popup_atevent                  = -3
Pico.exev_pntr_up                   = 1
Pico.exev_pntr_down                 = 2
Pico.exev_noclick                   = 4
Pico.exev_pntr_move                 = 8
Pico.exev_key                       = 16
Pico.exev_char                      = 32
Pico.exev_toggle                    = 64
Pico.exev_exclusive                 = 128
Pico.exev_focus                     = 256
Pico.exev_no_hotspot                = 512
Pico.dir_horizontal                 = 0
Pico.dir_vertical                   = 90
Pico.dir_antihorizontal             = 180
Pico.dir_antivertical               = 270
Pico.eventcoding_param              = 0
Pico.eventcoding_xy                 = 256
Pico.eventcoding_pntr               = 512
Pico.eventcoding_data               = 768
Pico.eventcoding_kbd                = 1024
Pico.eventcodingmask                = 3840
Pico.nwe                            = 4096
Pico.we_activate                    = 1
Pico.we_deactivate                  = 2
Pico.we_close                       = 3
Pico.we_focus                       = 4
Pico.we_pntr_down                   = 516
Pico.we_pntr_up                     = 517
Pico.we_pntr_release                = 518
Pico.we_data                        = 774
Pico.we_resize                      = 263
Pico.we_build                       = 264
Pico.we_pntr_move                   = 521
Pico.we_kbd_char                    = 1034
Pico.we_kbd_keyup                   = 1035
Pico.we_kbd_keydown                 = 1036
Pico.we_appmsg                      = 769
Pico.nwe_theme_inserted             = 4097
Pico.nwe_theme_removed              = 4098
Pico.nwe_infilter                   = 4866
Pico.t_timer                        = 1
Pico.t_pntr_relative                = 2
Pico.t_activate                     = 8
Pico.t_deactivate                   = 16
Pico.t_keyup                        = 32
Pico.t_keydown                      = 64
Pico.t_release                      = 128
Pico.t_up                           = 256
Pico.t_down                         = 512
Pico.t_move                         = 1024
Pico.t_enter                        = 2048
Pico.t_leave                        = 4096
Pico.t_drag                         = 8192
Pico.t_char                         = 16384
Pico.t_stream                       = 32768
Pico.t_key_start                    = 65536
Pico.t_nontoolbar                   = 131072
Pico.t_pntr_status                  = 262144
Pico.t_key                          = 524288
Pico.t_scrollwheel                  = 1048576
Pico.t_touchscreen                  = 2097152
Pico.t_ts_calibrate                 = 4194304
Pico.kf_focused                     = 1
Pico.kf_child_focused               = 2
Pico.kf_container_focused           = 4
Pico.kf_always                      = 8
Pico.kf_app_topmost                 = 16
pgcf_text_ascii                     = 32
pgcf_text_acs                       = 64
pgcf_alpha                          = 128
pgcf_mask                           = -2147483648
Pico.usualcolor.black               = 0
Pico.usualcolor.green               = 32768
Pico.usualcolor.silver              = 12632256
Pico.usualcolor.lime                = 65280
Pico.usualcolor.gray                = 8421504
Pico.usualcolor.olive               = 8421376
Pico.usualcolor.white               = 16777215
Pico.usualcolor.yellow              = 16776960
Pico.usualcolor.maroon              = 8388608
Pico.usualcolor.navy                = 128
Pico.usualcolor.red                 = 16711680
Pico.usualcolor.blue                = 255
Pico.usualcolor.purple              = 8388736
Pico.usualcolor.teal                = 32896
Pico.usualcolor.fuchsia             = 16711935
Pico.usualcolor.aqua                = 65535
Pico.res_default_font               = 0
Pico.res_string_ok                  = 1
Pico.res_string_cancel              = 2
Pico.res_string_yes                 = 3
Pico.res_string_no                  = 4
Pico.res_string_segfault            = 5
Pico.res_string_matherr             = 6
Pico.res_string_pguierr             = 7
Pico.res_string_pguiwarn            = 8
Pico.res_string_pguierrdlg          = 9
Pico.res_string_pguicompat          = 10
Pico.res_default_textcolors         = 11
Pico.res_infilter_touchscreen       = 12
Pico.res_infilter_key_preprocess    = 13
Pico.res_infilter_pntr_preprocess   = 14
Pico.res_infilter_magic             = 15
Pico.res_infilter_key_dispatch      = 16
Pico.res_infilter_pntr_dispatch     = 17
Pico.res_default_cursorbitmap       = 18
Pico.res_default_cursorbitmask      = 19
Pico.res_background_widget          = 20
Pico.res_infilter_hotspot           = 21
Pico.res_infilter_key_alpha         = 22
Pico.res_infilter_pntr_normalize    = 23
Pico.res_num                        = 24
Pico.request_port                   = 30450
Pico.protocol_ver                   = 20
Pico.request_magic                  = 826366246
Pico.max_response_sz                = 12
Pico.response_err                   = 1
Pico.response_ret                   = 2
Pico.response_event                 = 3
Pico.response_data                  = 4
Pico.req_ping                       = 0
Pico.req_update                     = 1
Pico.req_mkwidget                   = 2
Pico.req_mkbitmap                   = 3
Pico.req_mkfont                     = 4
Pico.req_mkstring                   = 5
Pico.req_free                       = 6
Pico.req_set                        = 7
Pico.req_get                        = 8
Pico.req_mktheme                    = 9
Pico.req_mkcursor                   = 10
Pico.req_mkinfilter                 = 11
Pico.req_getresource                = 12
Pico.req_wait                       = 13
Pico.req_mkfillstyle                = 14
Pico.req_register                   = 15
Pico.req_unused_1                   = 16
Pico.req_sizetext                   = 17
Pico.req_batch                      = 18
Pico.req_regowner                   = 19
Pico.req_unregowner                 = 20
Pico.req_setmode                    = 21
Pico.req_getmode                    = 22
Pico.req_mkcontext                  = 23
Pico.req_rmcontext                  = 24
Pico.req_focus                      = 25
Pico.req_getstring                  = 26
Pico.req_dup                        = 27
Pico.req_setpayload                 = 28
Pico.req_getpayload                 = 29
Pico.req_chcontext                  = 30
Pico.req_writeto                    = 31
Pico.req_updatepart                 = 32
Pico.req_mkarray                    = 33
Pico.req_render                     = 34
Pico.req_newbitmap                  = 35
Pico.req_thlookup                   = 36
Pico.req_getinactive                = 37
Pico.req_setinactive                = 38
Pico.req_drivermsg                  = 39
Pico.req_loaddriver                 = 40
Pico.req_getfstyle                  = 41
Pico.req_findwidget                 = 42
Pico.req_checkevent                 = 43
Pico.req_sizebitmap                 = 44
Pico.req_appmsg                     = 45
Pico.req_createwidget               = 46
Pico.req_attachwidget               = 47
Pico.req_findthobj                  = 48
Pico.req_traversewgt                = 49
Pico.req_mktemplate                 = 50
Pico.req_setcontext                 = 51
Pico.req_getcontext                 = 52
Pico.req_infiltersend               = 53
Pico.req_mkshmbitmap                = 54
Pico.req_undef                      = 55
Pico.key_backspace                  = 8
Pico.key_tab                        = 9
Pico.key_clear                      = 12
Pico.key_return                     = 13
Pico.key_pause                      = 19
Pico.key_escape                     = 27
Pico.key_space                      = 32
Pico.key_exclaim                    = 33
Pico.key_quotedbl                   = 34
Pico.key_hash                       = 35
Pico.key_dollar                     = 36
Pico.key_percent                    = 37
Pico.key_ampersand                  = 38
Pico.key_quote                      = 39
Pico.key_leftparen                  = 40
Pico.key_rightparen                 = 41
Pico.key_asterisk                   = 42
Pico.key_plus                       = 43
Pico.key_comma                      = 44
Pico.key_minus                      = 45
Pico.key_period                     = 46
Pico.key_slash                      = 47
Pico.key_0                          = 48
Pico.key_1                          = 49
Pico.key_2                          = 50
Pico.key_3                          = 51
Pico.key_4                          = 52
Pico.key_5                          = 53
Pico.key_6                          = 54
Pico.key_7                          = 55
Pico.key_8                          = 56
Pico.key_9                          = 57
Pico.key_colon                      = 58
Pico.key_semicolon                  = 59
Pico.key_less                       = 60
Pico.key_equals                     = 61
Pico.key_greater                    = 62
Pico.key_question                   = 63
Pico.key_at                         = 64
Pico.key_leftbracket                = 91
Pico.key_backslash                  = 92
Pico.key_rightbracket               = 93
Pico.key_caret                      = 94
Pico.key_underscore                 = 95
Pico.key_backquote                  = 96
Pico.key_a                          = 97
Pico.key_b                          = 98
Pico.key_c                          = 99
Pico.key_d                          = 100
Pico.key_e                          = 101
Pico.key_f                          = 102
Pico.key_g                          = 103
Pico.key_h                          = 104
Pico.key_i                          = 105
Pico.key_j                          = 106
Pico.key_k                          = 107
Pico.key_l                          = 108
Pico.key_m                          = 109
Pico.key_n                          = 110
Pico.key_o                          = 111
Pico.key_p                          = 112
Pico.key_q                          = 113
Pico.key_r                          = 114
Pico.key_s                          = 115
Pico.key_t                          = 116
Pico.key_u                          = 117
Pico.key_v                          = 118
Pico.key_w                          = 119
Pico.key_x                          = 120
Pico.key_y                          = 121
Pico.key_z                          = 122
Pico.key_leftbrace                  = 123
Pico.key_pipe                       = 124
Pico.key_rightbrace                 = 125
Pico.key_tilde                      = 126
Pico.key_delete                     = 127
Pico.key_world_0                    = 160
Pico.key_world_1                    = 161
Pico.key_world_2                    = 162
Pico.key_world_3                    = 163
Pico.key_world_4                    = 164
Pico.key_world_5                    = 165
Pico.key_world_6                    = 166
Pico.key_world_7                    = 167
Pico.key_world_8                    = 168
Pico.key_world_9                    = 169
Pico.key_world_10                   = 170
Pico.key_world_11                   = 171
Pico.key_world_12                   = 172
Pico.key_world_13                   = 173
Pico.key_world_14                   = 174
Pico.key_world_15                   = 175
Pico.key_world_16                   = 176
Pico.key_world_17                   = 177
Pico.key_world_18                   = 178
Pico.key_world_19                   = 179
Pico.key_world_20                   = 180
Pico.key_world_21                   = 181
Pico.key_world_22                   = 182
Pico.key_world_23                   = 183
Pico.key_world_24                   = 184
Pico.key_world_25                   = 185
Pico.key_world_26                   = 186
Pico.key_world_27                   = 187
Pico.key_world_28                   = 188
Pico.key_world_29                   = 189
Pico.key_world_30                   = 190
Pico.key_world_31                   = 191
Pico.key_world_32                   = 192
Pico.key_world_33                   = 193
Pico.key_world_34                   = 194
Pico.key_world_35                   = 195
Pico.key_world_36                   = 196
Pico.key_world_37                   = 197
Pico.key_world_38                   = 198
Pico.key_world_39                   = 199
Pico.key_world_40                   = 200
Pico.key_world_41                   = 201
Pico.key_world_42                   = 202
Pico.key_world_43                   = 203
Pico.key_world_44                   = 204
Pico.key_world_45                   = 205
Pico.key_world_46                   = 206
Pico.key_world_47                   = 207
Pico.key_world_48                   = 208
Pico.key_world_49                   = 209
Pico.key_world_50                   = 210
Pico.key_world_51                   = 211
Pico.key_world_52                   = 212
Pico.key_world_53                   = 213
Pico.key_world_54                   = 214
Pico.key_world_55                   = 215
Pico.key_world_56                   = 216
Pico.key_world_57                   = 217
Pico.key_world_58                   = 218
Pico.key_world_59                   = 219
Pico.key_world_60                   = 220
Pico.key_world_61                   = 221
Pico.key_world_62                   = 222
Pico.key_world_63                   = 223
Pico.key_world_64                   = 224
Pico.key_world_65                   = 225
Pico.key_world_66                   = 226
Pico.key_world_67                   = 227
Pico.key_world_68                   = 228
Pico.key_world_69                   = 229
Pico.key_world_70                   = 230
Pico.key_world_71                   = 231
Pico.key_world_72                   = 232
Pico.key_world_73                   = 233
Pico.key_world_74                   = 234
Pico.key_world_75                   = 235
Pico.key_world_76                   = 236
Pico.key_world_77                   = 237
Pico.key_world_78                   = 238
Pico.key_world_79                   = 239
Pico.key_world_80                   = 240
Pico.key_world_81                   = 241
Pico.key_world_82                   = 242
Pico.key_world_83                   = 243
Pico.key_world_84                   = 244
Pico.key_world_85                   = 245
Pico.key_world_86                   = 246
Pico.key_world_87                   = 247
Pico.key_world_88                   = 248
Pico.key_world_89                   = 249
Pico.key_world_90                   = 250
Pico.key_world_91                   = 251
Pico.key_world_92                   = 252
Pico.key_world_93                   = 253
Pico.key_world_94                   = 254
Pico.key_world_95                   = 255
Pico.key_kp0                        = 256
Pico.key_kp1                        = 257
Pico.key_kp2                        = 258
Pico.key_kp3                        = 259
Pico.key_kp4                        = 260
Pico.key_kp5                        = 261
Pico.key_kp6                        = 262
Pico.key_kp7                        = 263
Pico.key_kp8                        = 264
Pico.key_kp9                        = 265
Pico.key_kp_period                  = 266
Pico.key_kp_divide                  = 267
Pico.key_kp_multiply                = 268
Pico.key_kp_minus                   = 269
Pico.key_kp_plus                    = 270
Pico.key_kp_enter                   = 271
Pico.key_kp_equals                  = 272
Pico.key_up                         = 273
Pico.key_down                       = 274
Pico.key_right                      = 275
Pico.key_left                       = 276
Pico.key_insert                     = 277
Pico.key_home                       = 278
Pico.key_end                        = 279
Pico.key_pageup                     = 280
Pico.key_pagedown                   = 281
Pico.key_f1                         = 282
Pico.key_f2                         = 283
Pico.key_f3                         = 284
Pico.key_f4                         = 285
Pico.key_f5                         = 286
Pico.key_f6                         = 287
Pico.key_f7                         = 288
Pico.key_f8                         = 289
Pico.key_f9                         = 290
Pico.key_f10                        = 291
Pico.key_f11                        = 292
Pico.key_f12                        = 293
Pico.key_f13                        = 294
Pico.key_f14                        = 295
Pico.key_f15                        = 296
Pico.key_numlock                    = 300
Pico.key_capslock                   = 301
Pico.key_scrollock                  = 302
Pico.key_rshift                     = 303
Pico.key_lshift                     = 304
Pico.key_rctrl                      = 305
Pico.key_lctrl                      = 306
Pico.key_ralt                       = 307
Pico.key_lalt                       = 308
Pico.key_rmeta                      = 309
Pico.key_lmeta                      = 310
Pico.key_lsuper                     = 311
Pico.key_rsuper                     = 312
Pico.key_mode                       = 313
Pico.key_help                       = 315
Pico.key_print                      = 316
Pico.key_sysreq                     = 317
Pico.key_break                      = 318
Pico.key_menu                       = 319
Pico.key_power                      = 320
Pico.key_euro                       = 321
Pico.key_alpha                      = 322
Pico.key_max                        = 322
Pico.mod_lshift                     = 1
Pico.mod_rshift                     = 2
Pico.mod_shift                      = 3
Pico.mod_lctrl                      = 64
Pico.mod_rctrl                      = 128
Pico.mod_ctrl                       = 192
Pico.mod_lalt                       = 256
Pico.mod_ralt                       = 512
Pico.mod_alt                        = 768
Pico.mod_lmeta                      = 1024
Pico.mod_rmeta                      = 2048
Pico.mod_meta                       = 3072
Pico.mod_num                        = 4096
Pico.mod_caps                       = 8192
Pico.mod_mode                       = 16384
Pico.canvas_nuke                    = 1
Pico.canvas_grop                    = 2
Pico.canvas_execfill                = 3
Pico.canvas_findgrop                = 4
Pico.canvas_setgrop                 = 5
Pico.canvas_movegrop                = 6
Pico.canvas_mutategrop              = 7
Pico.canvas_defaultflags            = 8
Pico.canvas_gropflags               = 9
Pico.canvas_redraw                  = 10
Pico.canvas_incremental             = 11
Pico.canvas_scroll                  = 12
Pico.canvas_inputmapping            = 13
Pico.canvas_gridsize                = 14