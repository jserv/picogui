/* $Id: constants.h,v 1.2 2000/09/09 01:52:46 micahjd Exp $
 *
 * picogui/constants.h - various constants needed by client, server,
 *                       and application
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000 Micah Dowty <micah@homesoftware.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * 
 * Contributors:
 * 
 * 
 * 
 */

#ifndef _H_PG_CONSTANTS
#define _H_PG_CONSTANTS

/******************** Keyboard constants */

/* Lots of these, so use a seperate file... */
#include <picogui/pgkeys.h>

/******************** Application manager */

/* Values for application type */
#define PG_APP_NORMAL  1
#define PG_APP_TOOLBAR 2
#define PG_APPMAX      2

/******************** Layout */

/* Alignment types */
#define PG_A_CENTER   0
#define PG_A_TOP      1
#define PG_A_LEFT     2
#define PG_A_BOTTOM   3
#define PG_A_RIGHT    4
#define PG_A_NW       5
#define PG_A_SW       6
#define PG_A_NE       7
#define PG_A_SE       8
#define PG_A_ALL      9
#define PG_AMAX       9   /* For error checking the range of the align */

/* Side values 
 * these come from the DIVNODE flags used internally in the
 * picoGUI server, hence the strange values
 */
#define PG_S_TOP      (1<<3)
#define PG_S_BOTTOM   (1<<4)
#define PG_S_LEFT     (1<<5)
#define PG_S_RIGHT    (1<<6)
#define PG_S_ALL      (1<<11)

/******************** Fonts */

/* Font style flags.  These flags can be used
 * when defining a font or in findfont
 */

#define PG_FSTYLE_FIXED        (1<<0)
#define PG_FSTYLE_DEFAULT      (1<<1)    /* The default font in its category,
					    fixed or proportional. */
#define PG_FSTYLE_SYMBOL       (1<<2)    /* Font contains special chars not letters
					    and will not be chosen unless
					    specifically asked for */
#define PG_FSTYLE_SUBSET       (1<<3)    /* Font does not contain all the ASCII
					    chars before 127, and shouldn't
					    be used unless asked for */
#define PG_FSTYLE_EXTENDED     (1<<4)    /* Contains international characters
					    above 127 and will be used if this
					    is requested */
#define PG_FSTYLE_IBMEXTEND    (1<<5)    /* Has IBM-PC extended characters */

/* These should only be used for findfont
 * (findfont uses them to add features to that individial
 * font descriptor, or to search for the best font)
 */
#define PG_FSTYLE_DOUBLESPACE  (1<<7)
#define PG_FSTYLE_BOLD         (1<<8)
#define PG_FSTYLE_ITALIC       (1<<9)
#define PG_FSTYLE_UNDERLINE    (1<<10)
#define PG_FSTYLE_STRIKEOUT    (1<<11)
#define PG_FSTYLE_GRAYLINE     (1<<12)  /* A faint underline */
#define PG_FSTYLE_FLUSH        (1<<14)  /* No margin */
#define PG_FSTYLE_DOUBLEWIDTH  (1<<15)
#define PG_FSTYLE_ITALIC2      (1<<16)  /* Twice the normal italic */

/******************** Errors */

/* Error types */
#define PG_ERRT_NONE     0x0000
#define PG_ERRT_MEMORY   0x0100
#define PG_ERRT_IO       0x0200
#define PG_ERRT_NETWORK  0x0300
#define PG_ERRT_BADPARAM 0x0400
#define PG_ERRT_HANDLE   0x0500
#define PG_ERRT_INTERNAL 0x0600
#define PG_ERRT_BUSY     0x0700

/******************** Handles */

/* Client's handle data type */
typedef unsigned long pghandle;

/* Data types */
#define PG_TYPE_BITMAP     1
#define PG_TYPE_WIDGET     2
#define PG_TYPE_FONTDESC   3
#define PG_TYPE_STRING     4

/******************** Themes (DO NOT USE- this will change!) */

/* Types applicable to elements */
#define PG_ELEM_NULL       0
#define PG_ELEM_FLAT       1   /* Uses c1. Depending on x,y,w,h it can be
			       a rectangle, slab, bar, or frame. */
#define PG_ELEM_GRADIENT   2   /* Uses c1,c2,angle,translucent */

/* Element states */
#define PG_STATE_ALL       255
#define PG_STATE_NORMAL    0
#define PG_STATE_HILIGHT   1
#define PG_STATE_ACTIVATE  2
#define PG_STATENUM        3

/* The theme structure (new elements must be added at the end) */

#define PG_E_BUTTON_BORDER     0
#define PG_E_BUTTON_FILL       1
#define PG_E_BUTTON_OVERLAY    2
#define PG_E_TOOLBAR_BORDER    3
#define PG_E_TOOLBAR_FILL      4
#define PG_E_SCROLLBAR_BORDER  5
#define PG_E_SCROLLBAR_FILL    6
#define PG_E_SCROLLIND_BORDER  7
#define PG_E_SCROLLIND_FILL    8
#define PG_E_SCROLLIND_OVERLAY 9
#define PG_E_INDICATOR_BORDER  10
#define PG_E_INDICATOR_FILL    11
#define PG_E_INDICATOR_OVERLAY 12
#define PG_E_PANEL_BORDER      13
#define PG_E_PANEL_FILL        14
#define PG_E_PANELBAR_BORDER   15
#define PG_E_PANELBAR_FILL     16
#define PG_E_POPUP_BORDER      17
#define PG_E_POPUP_FILL        18

#define PG_E_NUM 19

#define PG_EPARAM_WIDTH        1
#define PG_EPARAM_TYPE         2
#define PG_EPARAM_C1           3
#define PG_EPARAM_C2           4
#define PG_EPARAM_ANGLE        5
#define PG_EPARAM_TRANSLUCENT  6

/******************** Video */

/* Gropnode types (gropnodes are a single element in a metafile-like
 * structure to hold GRaphics OPerations)
 */
#define PG_GROP_NULL       0	/* Doesn't do anything - for temporarily
				 * turning something off, or for disabling
				 * unused features while keeping the grop
				 * node order constant */
#define PG_GROP_PIXEL      1	
#define PG_GROP_LINE   	   2
#define PG_GROP_RECT	   3
#define PG_GROP_FRAME      4
#define PG_GROP_SLAB       5
#define PG_GROP_BAR        6
#define PG_GROP_DIM        7
#define PG_GROP_TEXT       8
#define PG_GROP_BITMAP     9
#define PG_GROP_GRADIENT   10
#define PG_GROPMAX         10

/* Video mode flags */
#define PG_VID_FULLSCREEN     0x0001

/* Logical operations for blits */
#define PG_LGOP_NULL        0   /* Don't blit */
#define PG_LGOP_NONE        1   /* Blit, but don't use an LGOP */
#define PG_LGOP_OR          2
#define PG_LGOP_AND         3
#define PG_LGOP_XOR         4
#define PG_LGOP_INVERT      5
#define PG_LGOP_INVERT_OR   6
#define PG_LGOP_INVERT_AND  7
#define PG_LGOP_INVERT_XOR  8
#define PG_LGOPMAX          8   /* For error-checking */

/******************** Widgets */

/* Constants used for rship, the relationship between 
   a widget and its parent */
#define PG_DERIVE_BEFORE 0
#define PG_DERIVE_AFTER  1
#define PG_DERIVE_INSIDE 2

/* Types of widgets (in the same order they are
   in the table in widget.c) */
#define PG_WIDGET_TOOLBAR    0
#define PG_WIDGET_LABEL      1
#define PG_WIDGET_SCROLL     2
#define PG_WIDGET_INDICATOR  3
#define PG_WIDGET_BITMAP     4
#define PG_WIDGET_BUTTON     5
#define PG_WIDGET_PANEL      6
#define PG_WIDGET_POPUP      7
#define PG_WIDGET_BOX        8
#define PG_WIDGET_FIELD      9
#define PG_WIDGETMAX         9    /* For error checking */
     
/* Widget properties */
#define PG_WP_SIZE        1
#define PG_WP_SIDE        2
#define PG_WP_ALIGN       3
#define PG_WP_BGCOLOR     4
#define PG_WP_COLOR       5
#define PG_WP_SIZEMODE    6
#define PG_WP_TEXT        7
#define PG_WP_FONT        8
#define PG_WP_TRANSPARENT 9
#define PG_WP_BORDERCOLOR 10
#define PG_WP_BITMAP      12
#define PG_WP_LGOP        13
#define PG_WP_VALUE       14
#define PG_WP_BITMASK     15
#define PG_WP_BIND        16
#define PG_WP_SCROLL      17    /* Scroll bar binds here on scrollable widgets */
#define PG_WP_VIRTUALH    18    /* Basically, the maximum vertical scroll */
#define PG_WP_HOTKEY      19

/* Constants for SIZEMODE */
#define PG_SZMODE_PIXEL         0
#define PG_SZMODE_PERCENT       (1<<2)    /* The DIVNODE_UNIT_PERCENT flag */
#define PG_SZMODEMASK           (~PG_SZMODE_PERCENT)

/******************** Events */

/* Widget events */
#define PG_WE_ACTIVATE    1     /* Gets focus (or for a non-focusing widget such
			           as a button, it has been clicked/selected  */
#define PG_WE_DEACTIVATE  2     /* Lost focus */
     
/* Non-widget events */
#define PG_NWE_KBD_CHAR    10   /* These are sent if the client has captured the */
#define PG_NWE_KBD_KEYUP   11   /* keyboard (or pointing device ) */
#define PG_NWE_KBD_KEYDOWN 12
#define PG_NWE_PNTR_MOVE   13
#define PG_NWE_PNTR_UP     14
#define PG_NWE_PNTR_DOWN   15


#endif /* __H_PG_CONSTANTS */
/* The End */
