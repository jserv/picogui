/* $Id: constants.h,v 1.1 2000/11/06 00:31:33 micahjd Exp $
 *
 * picogui/constants.h - various constants needed by client, server,
 *                       and application
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000 Micah Dowty <micahjd@users.sourceforge.net>
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

/**** Values for application type */
#define PG_APP_NORMAL  1
#define PG_APP_TOOLBAR 2
#define PG_APPMAX      2

/**** Extra parameters for applications */

/* a PG_S_* value for the initial side the app sticks to */
#define PG_APPSPEC_SIDE      1

/* a bitmask of acceptable sides for the app */
#define PG_APPSPEC_SIDEMASK  2

/* Initial width and height */
#define PG_APPSPEC_WIDTH     3
#define PG_APPSPEC_HEIGHT    4

/* Minimum and maximum size */
#define PG_APPSPEC_MINWIDTH  5
#define PG_APPSPEC_MAXWIDTH  6
#define PG_APPSPEC_MINHEIGHT 7
#define PG_APPSPEC_MAXHEIGHT 8

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
#define PG_ERRT_FILEFMT  0x0800

/* Reserved for client-side errors (not used by the server) */
#define PG_ERRT_CLIENT   0x8000

/******************** Handles */

/* Client's handle data type */
typedef unsigned long pghandle;

/* Data types */
#define PG_TYPE_BITMAP     1
#define PG_TYPE_WIDGET     2
#define PG_TYPE_FONTDESC   3
#define PG_TYPE_STRING     4
#define PG_TYPE_THEME      5

/******************** Theme constants */

/* Theme objects. These don't have to correspond to widgets or anything
 * else in PicoGUI.  A widget can have more than one theme object, or
 * theme objects can be used for things that aren't widgets, etc...
 *
 * A theme object is just a category to place a list of properties in,
 * but theme objects inherit properties from other theme objects
 * according to a built in hierarchy
 *
 * As a naming convention, these constants should start with PG_THO_ and
 * after that use underscores to represent subclassing. Bases are omitted
 * from names (otherwise they would be much longer, and if extra bases
 * were added the names would become misleading) so they don't strictly
 * follow the inheritance flow. The theme compiler uses '.' to subclass
 * (for us C weenies) so what is PGTH_O_FOO_MUMBLE is foo.mumble
 * in the theme compiler.
 */

#define PGTH_O_DEFAULT             0    /* Every theme object inherits this */
#define PGTH_O_BASE_INTERACTIVE    1    /* Base for interactive widgets */
#define PGTH_O_BASE_CONTAINER      2    /* Base for containers like toolbars */
#define PGTH_O_BUTTON              3    /* The button widget */
#define PGTH_O_BUTTON_HILIGHT      4    /* Button, hilighted when mouse is over */
#define PGTH_O_BUTTON_ON           5    /* Button, mouse is pressed */
#define PGTH_O_TOOLBAR             6    /* The toolbar widget */
#define PGTH_O_SCROLL              7    /* The non-movable part of a scrollbar widget */
#define PGTH_O_SCROLL_THUMB        8    /* The thumb (movable part of a scrollbar) */
#define PGTH_O_INDICATOR           9    /* The indicator widget */
#define PGTH_O_PANEL               10   /* The background portion of a panel */
#define PGTH_O_PANEL_BAR           11   /* The draggable titlebar of a panel */
#define PGTH_O_POPUP               12   /* Popup window */
#define PGTH_O_BACKGROUND          13   /* Background widget bitmap */
#define PGTH_O_BASE_STATIC         14   /* Base for widgets that just display info */
#define PGTH_O_BASE_TLCONTAINER    15   /* Top-level containers like popups and panels */ 
#define PGTH_O_THEMEINFO           16   /* Information about the theme that should be
					   loaded into memory, like the name */

/* If you add a themeobject, be sure to increment this and add
   an inheritance entry in theme/thobjtab.c */
#define PGTH_ONUM                  16

/*** Loaders */

/* Property loaders perform some type of transformation 
   on the data value at load-time. */

#define PGTH_LOAD_NONE       0   /* Leave data as-is */
#define PGTH_LOAD_REQUEST    1   /* Treat data as a file-offset to load a request packet
				    from. This request packet is executed, and the
				    return code stored in 'data'. Errors cause an error
				    in loading the theme. */

/*** Property IDs */

/*                               Handle?
        Name              ID     | Data type   Description */

#define PGTH_P_BGCOLOR    1   /*   pgcolor     Default background color */
#define PGTH_P_FGCOLOR    2   /*   pgcolor     Default foreground color */
#define PGTH_P_BGFILL     3   /* H fillstyle   Background fill style    */
#define PGTH_P_OVERLAY    4   /* H fillstyle   Fill style applied last  */
#define PGTH_P_FONT       5   /* H fontdesc    A widget's main font     */
#define PGTH_P_NAME       6   /* H string      Name of something, like a theme */

/*** Tag IDs */

/* The name of the theme is not stored in a tag (because it is helpful
   to have the name of themes so the themes in use can be listed and
   manipulated) */

#define PGTH_TAG_AUTHOR        1
#define PGTH_TAG_AUTHOREMAIL   2
#define PGTH_TAG_URL           3
#define PGTH_TAG_README        4

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
#define PG_DERIVE_BEFORE_OLD  0    /* Deprecated version of PG_DERIVE_BEFORE */
#define PG_DERIVE_AFTER       1
#define PG_DERIVE_INSIDE      2
#define PG_DERIVE_BEFORE      3

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
