/* $Id$
 *
 * picogui/constants.h - various constants needed by client, server,
 *                       and application
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2003 Micah Dowty <micahjd@users.sourceforge.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 * 
 * Contributors:
 * 
 * 
 * 
 */

#ifndef _H_PG_CONSTANTS
#define _H_PG_CONSTANTS

/*! 
 * \file constants.h
 * \brief Constants used in the client and server
 * 
 * This file defines constants common to the PicoGUI client and the server.
 * Because the most interesting things in PicoGUI like widgets and gropnodes
 * are referred to using constants, they are documented in this file.
 * 
 * Usually this file does not need to be included
 * separately, it is included with <tt>\#include <picogui.h></tt>
 */

/*!
 * \defgroup constants Constants
 * 
 * These constants are used in both the PicoGUI client and the server.
 * They are defined in constants.h
 * 
 * \{
 */

/* Just to make sure... */
#ifndef NULL
#define NULL ((void*)0)
#endif

/******************** Keyboard constants */

/* Lots of these, so use a seperate file... */
#include <picogui/pgkeys.h>

/******************** Application manager */

/*!
 * \defgroup appconst Application registration constants
 * 
 * These constants are used with pgRegisterApp() to create
 * a new application
 * 
 * \{
 */

#define PG_APP_NORMAL   1     //!< Normal application for pgRegisterApp
#define PG_APP_TOOLBAR  2     //!< Toolbar application for pgRegisterApp
#define PG_APPMAX       2     //!< Current maximum value used in PG_APP_* constants

#define PG_APPSPEC_SIDE      1    //!< Force the app to a specified side
#define PG_APPSPEC_SIDEMASK  2    //!< A bitmask of acceptable sides for an application
#define PG_APPSPEC_WIDTH     3    //!< Requested width
#define PG_APPSPEC_HEIGHT    4    //!< Requested height

#define PG_APPSPEC_MINWIDTH  5    //!< Minimum allowed width
#define PG_APPSPEC_MAXWIDTH  6    //!< Maximum allowed width
#define PG_APPSPEC_MINHEIGHT 7    //!< Minimum allowed height
#define PG_APPSPEC_MAXHEIGHT 8    //!< Maximum allowed height

#define PG_OWN_DISPLAY       4    //!< Exclusive access to the display via pgRender

//! \}

/******************** Layout */

/*!
 * \defgroup layoutconst Layout constants
 * 
 * PicoGUI defines two types of layout constants:
 *  - PG_A_* constants define alignment, the way a smaller object is positioned
 *    within a larger one, usually without changing the smaller object's size.
 *    For example, placing text within a widget.
 *  - PG_S_* constants specify a side to attach something to, for example the
 *    side of a container that a widget will stick to
 *
 * The side flags can be combined with a bitwise 'or' if necessary.
 * The values of the PG_S_* constants are significant because they must
 * be compatible with the server's internal layout engine flags.
 * 
 * \sa PG_WP_SIDE, PG_WP_ALIGN
 *
 * \{
 */

/* Alignment types */
#define PG_A_CENTER   0  //!< Center in the available space
#define PG_A_TOP      1  //!< Stick to the top-center of the available space
#define PG_A_LEFT     2  //!< Stick to the left-center of the available space
#define PG_A_BOTTOM   3  //!< Stick to the bottom-center of the available space
#define PG_A_RIGHT    4  //!< Stick to the right-center of the available space
#define PG_A_NW       5  //!< Stick to the northwest corner
#define PG_A_SW       6  //!< Stick to the southwest corner
#define PG_A_NE       7  //!< Stick to the northeast corner
#define PG_A_SE       8  //!< Stick to the southeast corner
#define PG_A_ALL      9  //!< Occupy all available space (good for tiled bitmaps)
#define PG_AMAX       9  //!< Current maximum value for a PG_A_* constant 

#define PG_S_TOP      (1<<3)   //!< Stick to the top edge
#define PG_S_BOTTOM   (1<<4)   //!< Stick to the bottom edge
#define PG_S_LEFT     (1<<5)   //!< Stick to the left edge
#define PG_S_RIGHT    (1<<6)   //!< Stick to the right edget
#define PG_S_ALL      (1<<11)  //!< Occupy all available space

//! \}

/******************** Fonts */

/*!
 * \defgroup fontconst Font styles
 * 
 * These font style constants can be used as the \p flags parameter of
 * pgFindFont() to specify font attributes.
 * 
 * These constants can also be used when defining a new font for pgserver:
 *   - PG_FSTYLE_FIXED
 *   - PG_FSTYLE_DEFAULT
 *   - PG_FSTYLE_SYMBOL
 *   - PG_FSTYLE_SUBSET
 *   - PG_FSTYLE_ENCODING_ISOLATIN1
 *   - PG_FSTYLE_ENCODING_IBM
 *   - PG_FSTYLE_ENCODING_UNICODE
 * 
 * \{
 */

#define PG_FSTYLE_FIXED        (1<<0)    //!< Fixed width
#define PG_FSTYLE_DEFAULT      (1<<1)    //!< The default font in its category, fixed or proportional.
#define PG_FSTYLE_SYMBOL       (1<<2)    //!< Font contains nonstandard chars and will not be chosen unless specifically requested
#define PG_FSTYLE_SUBSET       (1<<3)    //!< Font does not contain all the ASCII chars before 127, and shouldn't be used unless requested
#define PG_FSTYLE_ENCODING_ISOLATIN1  (1<<4)  //!< ISO Latin-1 encoding
#define PG_FSTYLE_ENCODING_IBM        (1<<5)  //!< IBM-PC extended characters
#define PG_FSTYLE_DOUBLESPACE  (1<<7)    //!< Add extra space between lines
#define PG_FSTYLE_BOLD         (1<<8)    //!< Use or simulate a bold version of the font
#define PG_FSTYLE_ITALIC       (1<<9)    //!< Use or simulate an italic version of the font
#define PG_FSTYLE_UNDERLINE    (1<<10)   //!< Underlined text
#define PG_FSTYLE_STRIKEOUT    (1<<11)   //!< Strikeout, a line through the middle of the text
#define PG_FSTYLE_GRAYLINE     (1<<12)   //!< deprecated
#define PG_FSTYLE_FLUSH        (1<<14)   //!< Disable the margin that PicoGUI puts around text
#define PG_FSTYLE_DOUBLEWIDTH  (1<<15)   //!< Add extra space between characters
#define PG_FSTYLE_ITALIC2      (1<<16)   //!< Twice the slant of the default italic
#define PG_FSTYLE_ENCODING_UNICODE    (1<<17) //!< Unicode encoding
#define PG_FSTYLE_CONDENSED    (1<<18)   //!< Condensed width

#define PG_FSTYLE_ENCODING_MASK       (PG_FSTYLE_ENCODING_ISOLATIN1|\
                                       PG_FSTYLE_ENCODING_IBM|\
                                       PG_FSTYLE_ENCODING_UNICODE)

/* Styles that are part of the font, rather than how it's rendered */
#define PG_FSTYLE_STYLE_MASK          (PG_FSTYLE_BOLD|\
                                       PG_FSTYLE_ITALIC|\
                                       PG_FSTYLE_CONDENSED)

/* Styles that shouldn't be used unless requested */
#define PG_FSTYLE_TYPE_MASK          (PG_FSTYLE_SUBSET|\
                                      PG_FSTYLE_SYMBOL)

//! \}

/*!
 * \defgroup fontrep Font representations
 * 
 * These flags can be returned by pgGetFontStyle, indicating supported
 * methods of graphically representing a font.
 *
 * Currently this can only indicate whether a font has built-in bold, italic,
 * or bolditalic bitmaps, but in the future could be used to indicate whether
 * a style is bitmapped or scalable.
 * 
 * \{
 */

#define PG_FR_BITMAP_NORMAL        (1<<0)    //!< Normal bitmapped font
#define PG_FR_BITMAP_BOLD          (1<<1)    //!< Bitmapped font with bold
#define PG_FR_BITMAP_ITALIC        (1<<2)    //!< Bitmapped font with italic
#define PG_FR_BITMAP_BOLDITALIC    (1<<3)    //!< Bitmapped font with bold and italic
#define PG_FR_SCALABLE             (1<<4)    //!< TrueType, Type1, or other outline font

//! \}

/******************** Errors */

/*! 
 * \defgroup errconst Error types
 * 
 * These error types are passed to the error handler
 * when the client or server triggers an error.
 * 
 * \sa pgSetErrorHandler
 * 
 * \{
 */
#define PG_ERRT_NONE     0x0000    //!< No error condition
#define PG_ERRT_MEMORY   0x0100    //!< Error allocating memory
#define PG_ERRT_IO       0x0200    //!< Filesystem, operating system, or other IO error
#define PG_ERRT_NETWORK  0x0300    //!< Network (or IPC) communication error */
#define PG_ERRT_BADPARAM 0x0400    //!< Invalid parameters */
#define PG_ERRT_HANDLE   0x0500    //!< Invalid handle ID, type, or ownership
#define PG_ERRT_INTERNAL 0x0600    //!< Shouldn't happen (tell a developer!)
#define PG_ERRT_BUSY     0x0700    //!< Try again later?
#define PG_ERRT_FILEFMT  0x0800    //!< Error in a loaded file format (theme files, bitmaps)

#define PG_ERRT_CLIENT   0x8000    //!< An error caused by the client lib, not the server

//! \}

/******************** Handles */

/*!
 * \defgroup handletypes Handle types
 * \{
 */

/*!
 * \brief PicoGUI handle data type
 * 
 * A handle is an arbitrary number that refers to an object stored in the
 * PicoGUI server's memory. All handles have a type, all handles can be
 * deleted, and all handles can store a payload.
 * 
 * \sa pgDelete, PG_ERRT_HANDLE, pgSetPayload, pgGetPayload
 */
typedef unsigned long pghandle;

/* Data types */
#define PG_TYPE_BITMAP     1    //!< Created by pgNewBitmap()
#define PG_TYPE_WIDGET     2    //!< Created by pgNewWidget(), pgNewPopup(), pgNewPopupAt(), or pgRegisterApp()
#define PG_TYPE_FONTDESC   3    //!< Created by pgNewFont()
#define PG_TYPE_PGSTRING   4    //!< Created by pgNewString()
#define PG_TYPE_THEME      5    //!< Created by pgLoadTheme()
#define PG_TYPE_FILLSTYLE  6    //!< Used internally to store a theme's fillstyles
#define PG_TYPE_ARRAY      7    //!< Created by pgNewArray()
#define PG_TYPE_DRIVER     8    //!< Created by pgLoadDriver()
#define PG_TYPE_PALETTE    9    //!< An array of pgcolors, transformed into hwrcolors
#define PG_TYPE_WT         11   //!< Created by pgLoadWidgetTemplate
#define PG_TYPE_INFILTER   12   //!< One filter in the input filter chain
#define PG_TYPE_CURSOR     13   //!< Cursor object, used with input filters
#define PG_TYPE_PARAGRAPH  14   //!< A paragraph (wrapped text) 
#define PG_TYPE_DIVTREE    15   //!< Tree of divnodes, one layer of nonoverlapping items

/* Also add new handle types to the debug code in handle.c, r_handle_dump() */

#define PG_TYPEMASK        0x1F

//! \}

/******************** Theme constants */

/*!
 * \defgroup themeconst Theme constants
 * 
 * Themes in PicoGUI make use of many types of constants. Theme objects,
 * properties, tags, loaders, and opcodes are described in this section.
 * 
 * \{
 */

/*! 
 * \defgroup thobj Theme objects
 * 
 * Theme objects: they don't have to correspond to widgets or anything
 * else in PicoGUI, although they usually do.
 * A widget can have more than one theme object, or
 * theme objects can be used for things that aren't widgets, etc...
 *
 * A theme object is just a category to place a list of properties in,
 * but theme objects inherit properties from other theme objects
 * according to a built in hierarchy
 *
 * As a naming convention, these constants should start with PGTH_O_ and
 * after that use underscores to represent subclassing. Bases are omitted
 * from names (otherwise they would be much longer, and if extra bases
 * were added the names would become misleading) so they don't strictly
 * follow the inheritance flow. The theme compiler uses '.' to subclass
 * (for us C weenies) so what is PGTH_O_FOO_MUMBLE is foo.mumble
 * in the theme compiler.
 * 
 * \{
 */

#define PGTH_O_DEFAULT               0    //!< Every theme object inherits this 
#define PGTH_O_BASE_INTERACTIVE      1    //!< Base for interactive widgets 
#define PGTH_O_BASE_CONTAINER        2    //!< Base for containers like toolbars 
#define PGTH_O_BUTTON                3    //!< The button widget 
#define PGTH_O_BUTTON_HILIGHT        4    //!< Button, hilighted when mouse is over 
#define PGTH_O_BUTTON_ON             5    //!< Button, mouse is pressed 
#define PGTH_O_TOOLBAR               6    //!< The toolbar widget 
#define PGTH_O_SCROLL                7    //!< The scrollbar widget 
#define PGTH_O_SCROLL_HILIGHT        8    //!< Scroll, when mouse is over it 
#define PGTH_O_INDICATOR             9    //!< The indicator widget 
#define PGTH_O_PANEL                 10   //!< The background portion of a panel 
#define PGTH_O_PANELBAR              11   //!< The draggable titlebar of a panel 
#define PGTH_O_POPUP                 12   //!< Popup window 
#define PGTH_O_BACKGROUND            13   //!< Background widget bitmap 
#define PGTH_O_BASE_DISPLAY          14   //!< Base for widgets that mostly display stuff 
#define PGTH_O_BASE_TLCONTAINER      15   //!< Top-level containers like popups, panels  
#define PGTH_O_THEMEINFO             16   //!< Information about the theme that should be loaded into memory, like the name 
#define PGTH_O_LABEL                 17   //!< The label widget 
#define PGTH_O_FIELD                 18   //!< The field widget 
#define PGTH_O_BITMAP                19   //!< The bitmap widget 
#define PGTH_O_SCROLL_ON             20   //!< Scroll, when mouse is down 
#define PGTH_O_LABEL_SCROLL          21   //!< A label, when bound to a scrollbar 
#define PGTH_O_PANELBAR_HILIGHT      22   //!< A panelbar, when mouse is inside it 
#define PGTH_O_PANELBAR_ON           23   //!< A panelbar, when mouse is down 
#define PGTH_O_BOX                   24   //!< The box widget 
#define PGTH_O_LABEL_DLGTITLE        25   //!< A label, used for a dialog box title 
#define PGTH_O_LABEL_DLGTEXT         26   //!< A label, used for the body of a dialog 
#define PGTH_O_CLOSEBTN              27   //!< A panelbar close button 
#define PGTH_O_CLOSEBTN_ON           28   //!< A panelbar close button, mouse down 
#define PGTH_O_CLOSEBTN_HILIGHT      29   //!< A panelbar close button, mouse over 
#define PGTH_O_BASE_PANELBTN         30   //!< Base for a panelbar button 
#define PGTH_O_ROTATEBTN             31   //!< A panelbar rotate button 
#define PGTH_O_ROTATEBTN_ON          32   //!< A panelbar rotate button, mouse down 
#define PGTH_O_ROTATEBTN_HILIGHT     33   //!< A panelbar rotate button, mouse over 
#define PGTH_O_ZOOMBTN               34   //!< A panelbar zoom button 
#define PGTH_O_ZOOMBTN_ON            35   //!< A panelbar zoom button, mouse down 
#define PGTH_O_ZOOMBTN_HILIGHT       36   //!< A panelbar zoom button, mouse over 
#define PGTH_O_POPUP_MENU            37   //!< A popup menu 
#define PGTH_O_POPUP_MESSAGEDLG      38   //!< A message dialog 
#define PGTH_O_MENUITEM              39   //!< Item in a popup menu (customized button) 
#define PGTH_O_MENUITEM_HILIGHT      40   //!< menuitem with the mouse over it 
#define PGTH_O_CHECKBOX              41   //!< Check box (customized button) 
#define PGTH_O_CHECKBOX_HILIGHT      42   //!< checkbox with mouse over it 
#define PGTH_O_CHECKBOX_ON           43   //!< checkbox when on 
#define PGTH_O_FLATBUTTON            44   //!< Flat button (customized button) 
#define PGTH_O_FLATBUTTON_HILIGHT    45   //!< flatbutton with mouse over it 
#define PGTH_O_FLATBUTTON_ON         46   //!< flatbutton with mouse down 
#define PGTH_O_LISTITEM              47   //!< Listitem (customized button)
#define PGTH_O_LISTITEM_HILIGHT      48   //!< Listitem with mouse over it
#define PGTH_O_LISTITEM_ON           49   //!< Selected listitem
#define PGTH_O_CHECKBOX_ON_NOHILIGHT 50   //!< checkbox when on but not hilighted
#define PGTH_O_SUBMENUITEM           51   //!< Submenuitem
#define PGTH_O_SUBMENUITEM_HILIGHT   52   //!< Hilighted submenuitem
#define PGTH_O_RADIOBUTTON           53   //!< Radio button (cust. button)
#define PGTH_O_RADIOBUTTON_HILIGHT   54   //!< Radio button (cust. button)
#define PGTH_O_RADIOBUTTON_ON        55   //!< Radio button (cust. button)
#define PGTH_O_RADIOBUTTON_ON_NOHILIGHT 56 //!< Radio button (cust. button)
#define PGTH_O_TEXTBOX               57   //!< Textbox widget
#define PGTH_O_TERMINAL              58   //!< Terminal widget
#define PGTH_O_MENUBUTTON            60   //!< DSPLinux Application Menu
#define PGTH_O_MENUBUTTON_ON         61   //!< DSPLinux Application Menu
#define PGTH_O_MENUBUTTON_HILIGHT    62   //!< DSPLinux Application Menu
#define PGTH_O_LABEL_HILIGHT         63   //!< Label hilight or select - See PG_WP_HILIGHTED
#define PGTH_O_BOX_HILIGHT           64   //!< Box hilight or select - See PG_WP_HILIGHTED
#define PGTH_O_INDICATOR_H           65   //!< Horizontal indicator
#define PGTH_O_INDICATOR_V           66   //!< Vertical indicator
#define PGTH_O_SCROLL_H              67   //!< Horizontal scrollbar
#define PGTH_O_SCROLL_V              68   //!< Vertical scrollbar
#define PGTH_O_SCROLL_H_ON           69   //!< Clicked horizontal scrollbar
#define PGTH_O_SCROLL_H_HILIGHT      70   //!< Hilighted horizontal scrollbar
#define PGTH_O_SCROLL_V_ON           71   //!< Clicked vertical scrollbar
#define PGTH_O_SCROLL_V_HILIGHT      72   //!< Hilighted vertical scrollbar
#define PGTH_O_PANELBAR_H            73   //!< Horizontal panelbar
#define PGTH_O_PANELBAR_V            74   //!< Vertical panelbar
#define PGTH_O_PANELBAR_H_ON         75   //!< Clicked horizontal panelbar
#define PGTH_O_PANELBAR_H_HILIGHT    76   //!< Hilighted horizontal panelbar
#define PGTH_O_PANELBAR_V_ON         77   //!< Clicked vertical panelbar
#define PGTH_O_PANELBAR_V_HILIGHT    78   //!< Hilighted vertical panelbar
#define PGTH_O_TEXTEDIT              79   //!< textedit widget
#define PGTH_O_MANAGEDWINDOW         80   //!< Window managed by a host GUI
#define PGTH_O_TAB_PAGE              81   //!< The body of a tab page
#define PGTH_O_TAB_BAR               82   //!< The bar containing tabs
#define PGTH_O_TAB                   83   //!< An inactive tab
#define PGTH_O_TAB_HILIGHT           84   //!< A tab with the mouse over it
#define PGTH_O_TAB_ON                85   //!< An active tab
#define PGTH_O_TAB_ON_NOHILIGHT      86   //!< An active tab without the mouse over it

//! If you add a themeobject, be sure to increment this and add an inheritance entry in theme/memtheme.c
#define PGTH_ONUM                    87

//! Using a theme object named "custom" makes the server automatically allocate a free theme object at load time
#define PGTH_O_CUSTOM                0x7FFF

//! \}

/*** Loaders */

/*!
 * \defgroup thloadconst Theme loaders
 *
 * Theme loaders perform some type of transformation on a property's
 * value as it is loaded.
 *
 * \{
 */

#define PGTH_LOAD_NONE       0   //!< Leave data as-is
/*! 
 * \brief Load a request packet
 *
 * Treat data as a file-offset to load a request packet
 * from. This request packet is executed, and the
 * return code stored as the new property value.
 * Errors in processing the request cause an error
 * in loading the theme.
 */
#define PGTH_LOAD_REQUEST    1
/*!
 * \brief Copy an existing property value
 * 
 * The property data is treated as a theme object and property
 * pair. A theme lookup is performed, and the result stored
 * as the property's value.
 */
#define PGTH_LOAD_COPY       2   
/*!
 * \brief Find a theme object by name
 *
 * The property data is a file-offset to a zero-terminated
 * string representing the name of a theme object.
 * The theme object's ID is used as the property's new value.
 * This could be accomplished using PGTH_LOAD_REQUEST and 
 * PGREQ_FINDTHOBJ, but that wouldn't work for loding from
 * the current theme.
 */
#define PGTH_LOAD_FINDTHOBJ  3

//! \}

/*** Property IDs */

/*!
 * \defgroup themeprop Theme properties
 *
 * The descriptions here are only guidelines. Many of these properties
 * are not used by the server itself, merely assigned IDs for the
 * use of the themes themselves (from within fillstyles, for example)
 * 
 * \{
 */

#define PGTH_P_BGCOLOR       1   //!< Default background color 
#define PGTH_P_FGCOLOR       2   //!< Default foreground color 
#define PGTH_P_BGFILL        3   //!< Background fillstyle    
#define PGTH_P_OVERLAY       4   //!< Fillstyle for scroll thumbs, the filled portion of an indicator  
#define PGTH_P_FONT          5   //!< A widget's main font     
#define PGTH_P_NAME          6   //!< Name of the theme object, themes can be searched by this
#define PGTH_P_WIDTH         7   //!< Reccomended width 
#define PGTH_P_HEIGHT        8   //!< Reccomended height 
#define PGTH_P_MARGIN        9   //!< The border in some objects 
#define PGTH_P_HILIGHTCOLOR  10  //!< Color for hilighting an object 
#define PGTH_P_SHADOWCOLOR   11  //!< Color for shading an object 
#define PGTH_P_OFFSET        12  //!< An amount to displace something by 
#define PGTH_P_ALIGN         13  //!< How to position an object's contents
#define PGTH_P_BITMAPSIDE    14  //!< Bitmap side relative to text (button) 
#define PGTH_P_BITMAPMARGIN  15  //!< Spacing between bitmap and text 
#define PGTH_P_BITMAP1       16  //!< Generic bitmap property for theme use 
#define PGTH_P_BITMAP2       17  //!< Generic bitmap property for theme use 
#define PGTH_P_BITMAP3       18  //!< Generic bitmap property for theme use 
#define PGTH_P_BITMAP4       19  //!< Generic bitmap property for theme use 
#define PGTH_P_SPACING       20  //!< Distance between similar widgets 
#define PGTH_P_TEXT          21  //!< Text caption for something like a button 
#define PGTH_P_SIDE          22  //!< Side for a widget or subwidget 
#define PGTH_P_BACKDROP      23  //!< Fillstyle on the screen behind a popup 
#define PGTH_P_WIDGETBITMAP  24  //!< Bitmap for something like a button 
#define PGTH_P_WIDGETBITMASK 25  //!< Bitmask for something like a button 
#define PGTH_P_CURSORBITMAP  26  //!< Bitmap for the (mouse) pointer 
#define PGTH_P_CURSORBITMASK 27  //!< Bitmask for the (mouse) pointer 
#define PGTH_P_HIDEHOTKEYS   28  //!< Set to a PG_HHK_* constant
#define PGTH_P_ATTR_DEFAULT  29  //!< Default attribute for the terminal
#define PGTH_P_ATTR_CURSOR   30  //!< Default attribute for the terminal
#define PGTH_P_TEXTCOLORS    31  //!< Text color pallete for the terminal
#define PGTH_P_TIME_ON       32  //!< Milliseconds on for flashing cursor
#define PGTH_P_TIME_OFF      33  //!< Milliseconds off for flashing cursor
#define PGTH_P_TIME_DELAY    34  //!< Milliseconds to wait before flashing
#define PGTH_P_PARENT        35  //!< Overrides the default theme inheritance
#define PGTH_P_XOFFSET       36  //!< General purpose
#define PGTH_P_YOFFSET       37  //!< General purpose
#define PGTH_P_TICKS         38  //!< The time in millisecond ticks (use with caution)
#define PGTH_P_CRSRHOTSPOT_X 39  //!< Hotspot X position on the mouse cursor
#define PGTH_P_CRSRHOTSPOT_Y 40  //!< Hotspot Y position on the mouse cursor
#define PGTH_P_CURSOR_WIDTH  41  //!< Width of the text editing cursor 
#define PGTH_P_BORDER_SIZE   42  //!< Width of outer border on a panel widget
#define PGTH_P_BORDER_FILL   43  //!< Fillstyle for the outer border
#define PGTH_P_OBJECT_ON     60  //!< Subobjects for the button widget */
#define PGTH_P_OBJECT_ON_NOHILIGHT 61
#define PGTH_P_OBJECT_HILIGHT 62
#define PGTH_P_RANDOM        63  //!< Random number generator (uses the rand() function) 

/* These should be removed when the standard dialogs are redone,
 * in favor of sticking to custom theme objects!
 */
#define PGTH_P_ICON_OK            1000   //!< Icon property (usually in PGTH_O_DEFAULT)
#define PGTH_P_ICON_OK_MASK       1001   //!< Icon property (usually in PGTH_O_DEFAULT)
#define PGTH_P_ICON_CANCEL        1002   //!< Icon property (usually in PGTH_O_DEFAULT)
#define PGTH_P_ICON_CANCEL_MASK   1003   //!< Icon property (usually in PGTH_O_DEFAULT)
#define PGTH_P_ICON_YES           1004   //!< Icon property (usually in PGTH_O_DEFAULT)
#define PGTH_P_ICON_YES_MASK      1005   //!< Icon property (usually in PGTH_O_DEFAULT)
#define PGTH_P_ICON_NO            1006   //!< Icon property (usually in PGTH_O_DEFAULT)
#define PGTH_P_ICON_NO_MASK       1007   //!< Icon property (usually in PGTH_O_DEFAULT)
#define PGTH_P_ICON_ERROR         1008   //!< Icon property (usually in PGTH_O_DEFAULT)
#define PGTH_P_ICON_ERROR_MASK    1009   //!< Icon property (usually in PGTH_O_DEFAULT)
#define PGTH_P_ICON_MESSAGE       1010   //!< Icon property (usually in PGTH_O_DEFAULT)
#define PGTH_P_ICON_MESSAGE_MASK  1011   //!< Icon property (usually in PGTH_O_DEFAULT)
#define PGTH_P_ICON_QUESTION      1012   //!< Icon property (usually in PGTH_O_DEFAULT)
#define PGTH_P_ICON_QUESTION_MASK 1013   //!< Icon property (usually in PGTH_O_DEFAULT)
#define PGTH_P_ICON_WARNING       1014   //!< Icon property (usually in PGTH_O_DEFAULT)
#define PGTH_P_ICON_WARNING_MASK  1015   //!< Icon property (usually in PGTH_O_DEFAULT)

#define PGTH_P_HOTKEY_OK          1501   //!< Hotkey property (usually in PGTH_O_DEFAULT) 
#define PGTH_P_HOTKEY_CANCEL      1502   //!< Hotkey property (usually in PGTH_O_DEFAULT) 
#define PGTH_P_HOTKEY_YES         1503   //!< Hotkey property (usually in PGTH_O_DEFAULT)   
#define PGTH_P_HOTKEY_NO          1504   //!< Hotkey property (usually in PGTH_O_DEFAULT) 
#define PGTH_P_HOTKEY_UP          1505   //!< Hotkey property (usually in PGTH_O_DEFAULT) 
#define PGTH_P_HOTKEY_DOWN        1506   //!< Hotkey property (usually in PGTH_O_DEFAULT) 
#define PGTH_P_HOTKEY_LEFT        1507   //!< Hotkey property (usually in PGTH_O_DEFAULT) 
#define PGTH_P_HOTKEY_RIGHT       1508   //!< Hotkey property (usually in PGTH_O_DEFAULT) 
#define PGTH_P_HOTKEY_ACTIVATE    1509   //!< Hotkey property (usually in PGTH_O_DEFAULT) 
#define PGTH_P_HOTKEY_NEXT        1510   //!< Hotkey property (usually in PGTH_O_DEFAULT) 

//! Application-defined theme properties start here
#define PGTH_P_USER               10000

//! Automatically allocated properties used only by the theme start here
#define PGTH_P_THEMEAUTO          20000

#define PG_HHK_NONE          0           //!< Do not hide any buttons
#define PG_HHK_RETURN_ESCAPE 1           //!< Hide buttons hotkeyed to return or escape

//! \}

/*** Tag IDs */

/*!
 * \defgroup thtagconst Theme tag constants
 * 
 * Theme tags are designed to store extra data about a theme that
 * does not need to be loaded into memory for the theme to
 * function. 
 *
 * The name of the theme is not stored in a tag, because it is helpful
 * to have the name of themes stored in memory so they can be referenced
 * more easily.
 *
 * \{ 
 */

#define PGTH_TAG_AUTHOR        1
#define PGTH_TAG_AUTHOREMAIL   2
#define PGTH_TAG_URL           3
#define PGTH_TAG_README        4

//! \}

/*** Fillstyle opcodes */

/*!
 * \defgroup fsops Fillstyle opcodes
 *
 * A fillstyle is a small stack-based program in a
 * theme that can perform theme lookups and create gropnodes,
 * among other things. The fillstyle code is compiled by \p themec
 * into a series of opcodes, described in this section.
 * 
 * When the fillstyle interpreter is expecting a new opcode,
 * there are five types of bytes it could find:
 * 
 * \verbatim
  
   Bits:  7 6 5 4 3 2 1 0
          
	  1 G G G G G G G    Build gropnode
          0 1 L L L L L L    Short numeric literal
	  0 0 1 C C C C C    Command code
          0 0 0 1 V V V V    Retrieve variable
	  0 0 0 0 V V V V    Set variable 

   L - numeric literal
   G - gropnode type
   V - variable offset
   C - command code constant

\endverbatim
 * 
 * These five types (corresponding to the PGTH_OPSIMPLE_* constants)
 * implement frequently-used commands in one byte. Longer opcodes
 * use PGTH_OPSIMPLE_CMDCODE to store a PGTH_OPCMD_* constant with the
 * actual opcode.
 * 
 * \{
 */

#define PGTH_OPSIMPLE_GROP       0x80   //!< Create a gropnode, 7-bit gropnode type
#define PGTH_OPSIMPLE_LITERAL    0x40   //!< Push a 6-bit numeric literal onto the stack
#define PGTH_OPSIMPLE_CMDCODE    0x20   //!< Container for a PG_OPCMD_* opcode
#define PGTH_OPSIMPLE_GET        0x10   //!< Push a copy of a variable at the indicated 4-bit stack offset
#define PGTH_OPSIMPLE_SET        0x00   //!< Pop a value off the stack and overwrite the indicated variable

/* Command codes */
#define PGTH_OPCMD_LONGLITERAL   0x20   //!< Followed by a 4-byte literal
#define PGTH_OPCMD_PLUS          0x21 
#define PGTH_OPCMD_MINUS         0x22
#define PGTH_OPCMD_MULTIPLY      0x23
#define PGTH_OPCMD_DIVIDE        0x24
#define PGTH_OPCMD_SHIFTL        0x25
#define PGTH_OPCMD_SHIFTR        0x26
#define PGTH_OPCMD_OR            0x27
#define PGTH_OPCMD_AND           0x28
#define PGTH_OPCMD_LONGGROP      0x29   //!< Followed by a 2-byte grop type
#define PGTH_OPCMD_LONGGET       0x2A   //!< Followed by a 1-byte var offset
#define PGTH_OPCMD_LONGSET       0x2B   //!< Followed by a 1-byte var offset
#define PGTH_OPCMD_PROPERTY      0x2C   //!< Followed by 2-byte object code and 2-byte property code
#define PGTH_OPCMD_LOCALPROP     0x2D   //!< Followed by 2-byte property code
#define PGTH_OPCMD_COLORADD      0x2F   //!< Add two pgcolors, clamping to white
#define PGTH_OPCMD_COLORSUB      0x30   //!< Subtract two pgcolors, clamping to black
#define PGTH_OPCMD_COLORMULT     0x31   //!< Multiply two pgcolors
#define PGTH_OPCMD_COLORDIV      0x32   //!< Divide two pgcolors
#define PGTH_OPCMD_QUESTIONCOLON 0x33   //!< The ?: conditional operator from C
#define PGTH_OPCMD_EQ            0x34
#define PGTH_OPCMD_LT            0x35
#define PGTH_OPCMD_GT            0x36
#define PGTH_OPCMD_LOGICAL_OR    0x37
#define PGTH_OPCMD_LOGICAL_AND   0x38
#define PGTH_OPCMD_LOGICAL_NOT   0x39
#define PGTH_OPCMD_WIDGET        0x3A   //!< Return a handle to the widget being drawn to
#define PGTH_OPCMD_TRAVERSEWGT   0x3B   //!< args: widget, direction, count (same as request)
#define PGTH_OPCMD_GETWIDGET     0x3C   //!< args: widget, property; returns value of property
#define PGTH_OPCMD_CALL          0x3D   //!< folowed by 2-byte thobj and 2-byte property, args: x,y,w,h
#define PGTH_OPCMD_LOCALCALL     0x3E   //!< folowed by 2-byte property, args: x,y,w,h
#define PGTH_OPCMD_EXTENDED      0x3F   //!< Extended command follows


/* Extended commands: these take two bytes
 */
#define PGTH_EXCMD_SKIP          0x01   //!< Move instruction pointer (relative to current position)
#define PGTH_EXCMD_SKIP_IF       0x02   //!< Like SKIP, but only if the top of the stack is nonzero

/* End fillstyles */
//! \}
/* End themes */
//! \}

/******************** Video */

/*!
 * \defgroup gropconst Gropnodes
 *
 * Gropnodes are the fundamental unit of rendering in PicoGUI, a single element
 * in a list of GRaphics OPerations.
 *
 * The most frequently used grops should be 7 bits or less to keep theme opcode
 * size at 8 bits. If it goes over 7 bits, the full 16 bits can be used
 * for the gropnode type and a 24 bit theme opcode will be used.
 * 
 * The LSB (bit 0) is a 'nonvisual' flag indicating it does no rendering,
 * only setup work. Bit 1 indicates that it is 'unpositioned' (does not use
 * x,y,w,h parameters) Bits 2 and 3 indicate how many extra parameters are
 * required. All other bits must be used to uniquely identify the gropnode
 * type.
 * 
 * Sorry if this seems a little paranoid, but the point is to save as much
 * space as possible in these structures as there will be many copies of them.
 *
 * See the Canvas widget documentation for more information on using the
 * gropnodes directly in a client program.
 * 
 * \sa PGCANVAS_GROP
 * 
 * \{
 */

#define PG_GROP_RECT          0x00   
#define PG_GROP_FRAME         0x10   
#define PG_GROP_SLAB          0x20   
#define PG_GROP_BAR           0x30   
#define PG_GROP_PIXEL         0x40
#define PG_GROP_LINE   	      0x50
#define PG_GROP_ELLIPSE       0x60 
#define PG_GROP_FELLIPSE      0x70
#define PG_GROP_TEXT          0x04   //!< Param: string 
#define PG_GROP_BITMAP        0x14   //!< Param: bitmap 
#define PG_GROP_TILEBITMAP    0x24   //!< Param: bitmap 
#define PG_GROP_FPOLYGON      0x34   //!< Param: array
#define PG_GROP_BLUR          0x44   //!< Param: radius
#define PG_GROP_PARAGRAPH     0x54   //!< Param: paragraph handle
#define PG_GROP_PARAGRAPH_INC 0x64   //!< Param: paragraph handle
#define PG_GROP_ROTATEBITMAP  0x74   //!< Param: bitmap 
#define PG_GROP_TEXTRECT      0x84   //!< Param: string 
#define PG_GROP_ARC           0x08   //!< Param: start angle, stop angle
#define PG_GROP_GRADIENT      0x0C   //!< Param: angle, c1, c2 
#define PG_GROP_TEXTGRID      0x1C   //!< Param: string, bufferw, offset
#define PG_GROP_NOP           0x03
#define PG_GROP_RESETCLIP     0x13   //!< Reset clip to whole divnode
#define PG_GROP_SETOFFSET     0x01   //!< this grop's rect sets offset
#define PG_GROP_SETCLIP       0x11   //!< this grop's rect sets clipping
#define PG_GROP_SETSRC        0x21   //!< this grop's rect sets src_*
#define PG_GROP_SETMAPPING    0x05   //!< Param: PG_MAP_* const
#define PG_GROP_SETCOLOR      0x07   //!< Param: pgcolor
#define PG_GROP_SETFONT       0x17   //!< Param: font
#define PG_GROP_SETLGOP       0x27   //!< Param: lgop
#define PG_GROP_SETANGLE      0x37   //!< Param: angle in degrees
#define PG_GROP_VIDUPDATE    0x800   //!< Forces a video update

//! Video-driver-defined grops are or'ed with this
#define PG_GROP_USER     0x1000

//! Find any gropnode's number of parameters
#define PG_GROPPARAMS(x)   (((x)>>2)&0x03)

//! Returns nonzero if the gropnode type specified does not actually draw something, only sets parameters
#define PG_GROP_IS_NONVISUAL(x)  ((x)&1)

//! Returns nonzero if the gropnode doesn't require position data (x,y,w,h)
#define PG_GROP_IS_UNPOSITIONED(x) ((x)&2)

/*!
 * \defgroup gropflags Gropnode flags
 * \{
 */

//! The gropnode can be scrolled using the divnode's translation (tx,ty)
#define PG_GROPF_TRANSLATE    (1<<0)
//! Rendered in incremental updates, not rendered normally
#define PG_GROPF_INCREMENTAL  (1<<1)
//! Always rendered, but this flag is cleared afterwards
#define PG_GROPF_PSEUDOINCREMENTAL (1<<2)
//! Always rendered, the gropnode is deleted afterwards
#define PG_GROPF_TRANSIENT    (1<<3)
//! The primitive's color is taken from its first parameter instead of the value set with PG_GROP_SETCOLOR
#define PG_GROPF_COLORED      (1<<4)
//! The gropnode is always rendered
#define PG_GROPF_UNIVERSAL    (1<<5)

//! \}

/*!
 * \defgroup gropmap Coordinate mapping
 * \sa PG_GROP_SETMAPPING, pgSetMapping
 * \{
 */

#define PG_MAP_NONE           0
/*!
 * This grop's width and height define
 * the virtual width and height of the
 * divnode, grops are mapped from this
 * to the real size
 */
#define PG_MAP_SCALE          1      
#define PG_MAP_SQUARESCALE    2      //!< Like PG_MAP_SCALE, but constrain the aspect ratio
/*!
 * The virtual coordinates still refer to pixels, but they are centered within the
 * actual output rectangle.
 */
#define PG_MAP_CENTER         3

//! \}

/*!
 * \defgroup lgopconst Logical Operations
 * 
 * These constants describe a method of combining
 * a new primitive with data already on the display
 * 
 * \sa pgSetLgop, PG_GROP_SETLGOP
 *
 * \{
 */

/* Logical operations for any primitive */
#define PG_LGOP_NULL        0   //!< Don't render the primitive
#define PG_LGOP_NONE        1   //!< Copy directly to the screen
#define PG_LGOP_OR          2
#define PG_LGOP_AND         3
#define PG_LGOP_XOR         4
#define PG_LGOP_INVERT      5   
#define PG_LGOP_INVERT_OR   6   //!< Inverts the source data beforehand
#define PG_LGOP_INVERT_AND  7
#define PG_LGOP_INVERT_XOR  8
#define PG_LGOP_ADD         9
#define PG_LGOP_SUBTRACT    10
#define PG_LGOP_MULTIPLY    11
#define PG_LGOP_STIPPLE     12
#define PG_LGOP_ALPHA       13  //!< Alpha blending using colors with the PGCF_ALPHA flag

#define PG_LGOPMAX          13  //!< For error-checking

//! \}

/* End gropnodes */
//! \}

/*!
 * \defgroup vidflags Video mode flags
 * 
 * Use these with pgSetVideoMode()
 * \{
 */

#define PG_VID_FULLSCREEN     0x0001  //!< Deprecated
#define PG_VID_DOUBLEBUFFER   0x0002  //!< Deprecated
#define PG_VID_ROOTLESS       0x0100

#define PG_VID_ROTATE90       0x0004  //!< Rotate flags are mutually exclusive
#define PG_VID_ROTATE180      0x0008
#define PG_VID_ROTATE270      0x0010
#define PG_VID_ROTATEMASK     0x001C  //!< Mask of all rotate flags

#define PG_VID_ROTBASE90      0x0020  //!< RotBase flags are mutually exclusive
#define PG_VID_ROTBASE180     0x0040
#define PG_VID_ROTBASE270     0x0080
#define PG_VID_ROTBASEMASK    0x00E0  //!< Mask of all rotation base flags

#define PG_FM_SET             0      //!< Sets all flags to specified value
#define PG_FM_ON              1      //!< Turns on specified flags
#define PG_FM_OFF             2      //!< Turns off specified flags
#define PG_FM_TOGGLE          3      //!< Toggles specified flags

//! \}

/*!
 * \defgroup bitformat Bitmap format flags
 *
 * Used to describe the hardware bitmap format in a pgshmbitmap structure
 * \{
 */

#define PG_BITFORMAT_ROTATE90    (1<<0)   //!< Indicates that the bitmap is stored pre-rotated
#define PG_BITFORMAT_ROTATE180   (1<<1)
#define PG_BITFORMAT_ROTATE270   (1<<2)

#define PG_BITFORMAT_GRAYSCALE   (1<<3)   //!< Each pixel only represents intensity
#define PG_BITFORMAT_INDEXED     (1<<4)   //!< Each pixel represents a palette index
#define PG_BITFORMAT_SYMBOLIC    (1<<5)   //!< Each pixel represents a character or symbol
#define PG_BITFORMAT_TRUECOLOR   (1<<6)   //!< Each pixel has red, green, and blue components
#define PG_BITFORMAT_ALPHA       (1<<7)   //!< Each pixel also has an alpha channel

//! \}

/*!
 * \defgroup drvmsgs Driver messages
 * 
 * These flags specify hardware-specific commands that can be
 * sent from driver to driver or from applciation to driver.
 *
 * \{
 */

#define PGDM_BACKLIGHT        2   //!< Turn the backlight on/off
#define PGDM_SOUNDFX          3   //!< Parameter is a PG_SND_* constant
#define PGDM_POWER            4   //!< Enter the power mode, PG_POWER_*
#define PGDM_SDC_CHAR         5   //!< Send a character to the secondary display channel
#define PGDM_BRIGHTNESS       6   //!< Set display brightness, 0x00-0xFF
#define PGDM_CONTRAST         7   //!< Set display contrast, 0x00-0xFF
#define PGDM_SIGNAL          13   //!< Internal message, sends SIGUSR1/2 to drivers (param is signal)
#define PGDM_READY           14   //!< Notify the drivers that the server is completely up
#define PGDM_MOUSEWARP       15   //!< Warp the mouse to the specified coordinates, X and Y packed into 32 bits

#define PG_SND_KEYCLICK       1   //!< Short click
#define PG_SND_BEEP           2   //!< Terminal beep
#define PG_SND_VISUALBELL     3   //!< Flash the visual bell if available
#define PG_SND_ALARM          4
#define PG_SND_SHORTBEEP      5   //!< Shorter beep

#define PG_POWER_OFF          0   //!< Turn completely off
#define PG_POWER_SLEEP       50   //!< Stop CPU, turn off peripherals
#define PG_POWER_VIDBLANK    70   //!< Blank the video output
#define PG_POWER_FULL       100   //!< Full speed

//! \}

/******************** Widgets */


/* Constants used for rship, the relationship between 
   a widget and its parent */
#define PG_DERIVE_BEFORE_OLD  0    /* Deprecated version of PG_DERIVE_BEFORE */
#define PG_DERIVE_AFTER       1
#define PG_DERIVE_INSIDE      2
#define PG_DERIVE_BEFORE      3

/* Constants used in pgTraverseWidget */
#define PG_TRAVERSE_CHILDREN  1    //!< Starting with this widget's first child, traverse forward
#define PG_TRAVERSE_FORWARD   2
#define PG_TRAVERSE_BACKWARD  3    //!< Going backwards is much slower than going forward right now
#define PG_TRAVERSE_CONTAINER 4    //!< 'count' is the number of container levels to traverse up
#define PG_TRAVERSE_APP       5    //!< Find the root widget owning this app, and traverse 'count' applications forward from it.

/* Types of widgets (in the same order they are
   in the table in widget.c) */
#define PG_WIDGET_TOOLBAR         0
#define PG_WIDGET_LABEL           1
#define PG_WIDGET_SCROLL          2
#define PG_WIDGET_INDICATOR       3
#define PG_WIDGET_MANAGEDWINDOW   4
#define PG_WIDGET_BUTTON          5
#define PG_WIDGET_PANEL           6  
#define PG_WIDGET_POPUP           7  
#define PG_WIDGET_BOX             8
#define PG_WIDGET_FIELD           9
#define PG_WIDGET_BACKGROUND      10 
#define PG_WIDGET_MENUITEM        11    /* A variation on button */
#define PG_WIDGET_TERMINAL        12    /* A full terminal emulator */
#define PG_WIDGET_CANVAS          13
#define PG_WIDGET_CHECKBOX        14    /* Another variation of button */
#define PG_WIDGET_FLATBUTTON      15    /* Yet another customized button */
#define PG_WIDGET_LISTITEM        16    /* Still yet another... */
#define PG_WIDGET_SUBMENUITEM     17    /* Menuitem with a submenu arrow */
#define PG_WIDGET_RADIOBUTTON     18    /* Like a check box, but exclusive */
#define PG_WIDGET_TEXTBOX         19    /* Client-side text layout */
#define PG_WIDGET_PANELBAR        20    /* Draggable bar and container */
#define PG_WIDGET_SIMPLEMENU      21    /* create a simple menu from a string or array */
#define PG_WIDGET_DIALOGBOX       22    /* A popup with a standard title */
#define PG_WIDGET_MESSAGEDIALOG   23    /* A popup that displays a message and gets a response */
#define PG_WIDGET_SCROLLBOX       24    /* A box widget including scroll bars */
#define PG_WIDGET_TEXTEDIT        25    /* Simple text editor */
#define PG_WIDGET_TABPAGE         26    /* A page in a tabbed book */
#define PG_WIDGETMAX              26    /* For error checking */

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
#define PG_WP_IMAGE       12
#define PG_WP_BITMAP      PG_WP_IMAGE   /* For backwards compatibility */
#define PG_WP_LGOP        13
#define PG_WP_VALUE       14
#define PG_WP_BITMASK     15
#define PG_WP_BIND        16
#define PG_WP_SCROLL_X    17    /* Horizontal and vertical scrolling amount */
#define PG_WP_SCROLL_Y    18
#define PG_WP_SCROLL      PG_WP_SCROLL_Y   /* For backwards compatibility */
#define PG_WP_HOTKEY      19
#define PG_WP_EXTDEVENTS  20    /* For buttons, a mask of extra events to send */
#define PG_WP_DIRECTION   21
#define PG_WP_ABSOLUTEX   22    /* read-only, relative to screen */
#define PG_WP_ABSOLUTEY   23
#define PG_WP_ON          24    /* on-off state of button/checkbox/etc */
#define PG_WP_THOBJ       25    /* Set a widget's theme object */
#define PG_WP_STATE       PG_WP_THOBJ    /* Deprecated! Use PG_WP_THOBJ instead */
#define PG_WP_NAME        26    /* A widget's name (for named containers, etc) */
#define PG_WP_PUBLICBOX   27    /* Set to 1 to allow other apps to make widgets
				 * in this container */
#define PG_WP_DISABLED    28    /* For buttons, grays out text and prevents clicking */
#define PG_WP_MARGIN      29    /* For boxes, overrides the default margin */
#define PG_WP_TEXTFORMAT  30    /* For the textbox, defines a format for  PG_WP_TEXT */
#define PG_WP_TRIGGERMASK 31    /* Mask of extra triggers accepted (self->trigger_mask) */
#define PG_WP_HILIGHTED   32    /* Widget property to hilight a widget and all it's children */
#define PG_WP_SELECTED    33    /* List property to select a row. */
#define PG_WP_SELECTED_HANDLE 34 /* List property to return a handle to the selected row */
#define PG_WP_AUTOSCROLL  35    /* For the textbox or terminal, scroll to any new
				 * text that's inserted */
#define PG_WP_LINES       36    /* Height, in lines */
#define PG_WP_PREFERRED_W 37    /* Read only (for now) properties to get any widget's preferred size */
#define PG_WP_PREFERRED_H 38
#define PG_WP_PANELBAR    39    /* Read-only property for panels returns a handle
				 * to its embedded panelbar widget */
#define PG_WP_AUTO_ORIENTATION 40 /* Automatically reorient child widgets when PG_WP_SIDE changes, see below */
#define PG_WP_THOBJ_BUTTON              41  /* These four theme properties set the theme objects used for the */
#define PG_WP_THOBJ_BUTTON_HILIGHT      42  /* three possible states of the button widget. */
#define PG_WP_THOBJ_BUTTON_ON           43
#define PG_WP_THOBJ_BUTTON_ON_NOHILIGHT 44
#define PG_WP_PANELBAR_LABEL  45  /* More read-only panelbar properties to get the built-in panelbar widgets */
#define PG_WP_PANELBAR_CLOSE  46
#define PG_WP_PANELBAR_ROTATE 47
#define PG_WP_PANELBAR_ZOOM   48
#define PG_WP_IMAGESIDE       49
#define PG_WP_BITMAPSIDE      PG_WP_IMAGESIDE   /* For backwards compatibility */
#define PG_WP_PASSWORD        50
#define PG_WP_HOTKEY_FLAGS    51  /* Keyboard event flags for the hotkey (PG_KF_*) */
#define PG_WP_HOTKEY_CONSUME  52  /* Flag indicating whether to consume the key event when a hotkey comes in */
#define PG_WP_WIDTH           53  /* A read only property for all widgets (use PG_WP_SIZE to change the size). Always in pixels */
#define PG_WP_HEIGHT          54  /* A read only property for all widgets (use PG_WP_SIZE to change the size). Always in pixels */
#define PG_WP_SPACING         55  /* Override the normal spacing between buttons */
#define PG_WP_MINIMUM         56  /* Used by the panelbar widget to set minimum size */
#define PG_WP_MULTILINE       57  /* Turn this off on the textbox widget to get a single line textbox */
#define PG_WP_SELECTION       58  /* Selected text in the textedit widget */
#define PG_WP_READONLY        59
#define PG_WP_INSERTMODE      60  /* Set to a PG_INSERT_* constant, for textbox widget */
#define PG_WP_TYPE            61  /* Return the type of the widget (PG_WIDGET_*) */
#define PG_WP_TAB             62  /* The tab associated with a tabpage widget */
#define PG_WP_TAB_BAR         63  /* The tab bar associated with a tabpage widget */
#define PG_WP_POPUP_IS_MENU   64  /* Popup is menu flag */
#define PG_WP_POPUP_IS_SUBMENU 65 /* Popup is submenu flag */
#define PG_WP_CURSOR_POSITION 66  /* Textbox cursor position */
#define PG_WP_HOTKEY_MODIFIERS 67 /* Modifiers to require along with the hotkey */

/* Constants for PG_WP_AUTO_ORIENTATION */
#define PG_AUTO_SIDE         (1<<0)
#define PG_AUTO_DIRECTION    (1<<1)

/* Constants for SIZEMODE */
#define PG_SZMODE_PIXEL         0
#define PG_SZMODE_PERCENT       (1<<2)    /* The DIVNODE_UNIT_PERCENT flag */
#define PG_SZMODE_CNTFRACT      (1<<15)   /* The DIVNODE_UNIT_CNTFRACT flag */
#define PG_SZMODEMASK           (PG_SZMODE_PERCENT|PG_SZMODE_PIXEL|PG_SZMODE_CNTFRACT)

/* Constants for PG_WP_INSERTMODE */
#define PG_INSERT_OVERWRITE   0   /* Overwrite the entire contents of the widget (Default) */
#define PG_INSERT_APPEND      1   /* Insert at the end of the document */
#define PG_INSERT_PREPEND     2   /* Insert at the beginning of the document */
#define PG_INSERT_ATCURSOR    3   /* Insert at the cursor */
#define PG_INSERTMAX          3   /* For error checking */

/* Constants for positioning a popup box */
#define PG_POPUP_CENTER   -1
#define PG_POPUP_ATCURSOR -2   /* (This also assumes it is a popup menu, and
				  uses PGTH_O_POPUP_MENU) */
#define PG_POPUP_ATEVENT  -3   /* (This also assumes it is a popup menu, and
				  uses PGTH_O_POPUP_MENU) */


/* Constants for PG_WP_EXTDEVENTS, to enable extra events */
#define PG_EXEV_PNTR_UP    0x0001
#define PG_EXEV_PNTR_DOWN  0x0002
#define PG_EXEV_NOCLICK    0x0004  /* (ignore clicks) in buttons */
#define PG_EXEV_PNTR_MOVE  0x0008
#define PG_EXEV_KEY        0x0010  /* Raw key events KEYUP and KEYDOWN */
#define PG_EXEV_CHAR       0x0020  /* Processed characters */
#define PG_EXEV_TOGGLE     0x0040  /* Clicks toggle the button's state */
#define PG_EXEV_EXCLUSIVE  0x0080  /* Button is mutually exclusive */
#define PG_EXEV_FOCUS      0x0100  /* We want to receive PG_WE_FOCUS */
#define PG_EXEV_NO_HOTSPOT 0x0200  /* Disable hotspot navigation for this button */
#define PG_EXEV_RESIZE     0x0400  /* Send WE_RESIZE when widget is rebuilt
				      (currently only works for buttons, the terminal
				       always sends this event) */

/* Constants for PG_WP_DIRECTION */
#define PG_DIR_HORIZONTAL     0
#define PG_DIR_VERTICAL       90
#define PG_DIR_ANTIHORIZONTAL 180
#define PG_DIR_ANTIVERTICAL   270

/******************** Events */

/* Events can return various types of data that the client library
 * will separate out for the app. To indicate a type of encoding, the
 * PG_WE_* constant is logically or'ed with one of these: */

#define PG_EVENTCODING_PARAM    0x000   /* Just a 32-bit parameter */
#define PG_EVENTCODING_XY       0x100   /* X,Y coordinates packed into param */
#define PG_EVENTCODING_PNTR     0x200   /* Mouse parameters (x,y,btn,chbtn) */
#define PG_EVENTCODING_DATA     0x300   /* Arbitrary data block */
#define PG_EVENTCODING_KBD      0x400   /* Keyboard params */
#define PG_EVENTCODINGMASK      0xF00

#define PG_NWE             0x1000       /* Flag indicating a non-widget event */

/* Widget events */
#define PG_WE_ACTIVATE     0x001 /* Button has been clicked/selected  */
#define PG_WE_DEACTIVATE   0x002 /* Sent when the user clicks outside the active popup */
#define PG_WE_CLOSE        0x003 /* A top-level widget has closed */
#define PG_WE_FOCUS        0x004 /* Sent when a button is focused, only if it has 
				  * PG_EXEV_FOCUS. The field widget always sends this. */
#define PG_WE_CHANGED      0x005 /* Sent by the textbox when its contents change
				    (but only after a delay) */
#define PG_WE_PNTR_DOWN    0x204 /* The "mouse" button is now down */
#define PG_WE_PNTR_UP      0x205 /* The "mouse" button is now up */
#define PG_WE_PNTR_RELEASE 0x206 /* The "mouse" button was released outside
				  * the widget */
#define PG_WE_PNTR_ENTER   0x207 /* The pointer is over the widget */
#define PG_WE_PNTR_LEAVE   0x208 /* The pointer is not over the widget */
#define PG_WE_DATA        0x306 /* Widget is streaming data to the app */
#define PG_WE_TITLECHANGE 0x307 /* Used by the terminal widget to send window title changes */
#define PG_WE_RESIZE      0x107 /* For terminal widgets */
#define PG_WE_BUILD       0x108 /* Sent from a canvas, clients can rebuild groplist */
#define PG_WE_SCROLLWHEEL 0x109 /* Sent from a canvas, includes x,y delta of scroll wheel */
#define PG_WE_PNTR_MOVE   0x209 /* The "mouse" moved */
#define PG_WE_KBD_CHAR    0x40A /* A focused keyboard character recieved */
#define PG_WE_KBD_KEYUP   0x40B /* A focused raw keyup event */
#define PG_WE_KBD_KEYDOWN 0x40C /* A focused raw keydown event */
#define PG_WE_APPMSG      0x301 /* Messages from another application */

/* Non-widget events */
#define PG_NWE_THEME_INSERTED 0x1001  /* A theme has been inserted into the theme system, 
				       * the parameter is the handle of that theme */
#define PG_NWE_THEME_REMOVED  0x1002  /* A theme has been removed the theme system, 
				       * the parameter is the handle of that theme.
				       * (Note that the handle will be invalid now, but it is
				       * provided for comparison if needed.) */
#define PG_NWE_INFILTER       0x1302  /* An incoming trigger for a client-side input filter.
				       * the data in this case is a pg_client_trigger union */

/* 'Triggers' are the notation used to describe events passing between input drivers
 * and widgets. These constants are used in input filters, and in specifying trigger masks
 * for the widgets.
 */
#define PG_TRIGGER_TIMER         (1<<0)  /* Timer event from install_timer */
#define PG_TRIGGER_PNTR_RELATIVE (1<<1)  /* Specify relative mouse motion and the current button status */
#define PG_TRIGGER_ACTIVATE      (1<<3)  /* Sent when it receives focus */
#define PG_TRIGGER_DEACTIVATE    (1<<4)  /* Losing focus */
#define PG_TRIGGER_KEYUP         (1<<5)  /* Ignores autorepeat, etc. Raw key codes*/
#define PG_TRIGGER_KEYDOWN       (1<<6)  /* Ditto. */
#define PG_TRIGGER_RELEASE       (1<<7)  /* Mouse up (see note) */
#define PG_TRIGGER_UP            (1<<8)  /* Mouse up in specified divnode */
#define PG_TRIGGER_DOWN          (1<<9)  /* Mouse down in divnode */
#define PG_TRIGGER_MOVE          (1<<10) /* Triggers on any mouse movement in node */
#define PG_TRIGGER_ENTER         (1<<11) /* Mouse moves inside widget */
#define PG_TRIGGER_LEAVE         (1<<12) /* Mouse moves outside widget */
#define PG_TRIGGER_DRAG          (1<<13) /* Mouse move when captured */
#define PG_TRIGGER_CHAR          (1<<14) /* A processed ASCII/Unicode character */
#define PG_TRIGGER_STREAM        (1<<15) /* Incoming packet (from WRITEDATA) */
#define PG_TRIGGER_KEY_START     (1<<16) /* Sent at the beginning of key propagation */
#define PG_TRIGGER_NONTOOLBAR    (1<<17) /* Not really a trigger, but widgets can put this
				          * in their trigger mask to request placement in
				          * the nontoolbar area when applicable */
#define PG_TRIGGER_PNTR_STATUS   (1<<18) /* A driver can send this trigger with the current
				          * status of the mouse to have the input filters
				          * automatically extrapolate other events. */
#define PG_TRIGGER_KEY           (1<<19) /* A driver can send this with a key code when
				          * the exact state of the key is unknown, to have
				          * KEYUP, KEYDOWN, and CHAR events generated. */
#define PG_TRIGGER_SCROLLWHEEL   (1<<20) /* The x,y coordinates passed with this are signed scroll
					  * wheel values */
#define PG_TRIGGER_TOUCHSCREEN   (1<<21) /* A touchscreen event to be processed by infilter_touchscreen */
#define PG_TRIGGER_TS_CALIBRATE  (1<<22) /* Store the touchscreen calibration given in this event */
#define PG_TRIGGER_CLOSE         (1<<23) /* Sent by drivers to a managed window when externally closed */
#define PG_TRIGGER_COMMAND       (1<<24) /* A command sent to a widget, for example by pgWriteCmd */
#define PG_TRIGGER_MOTIONTRACKER (1<<25) /* Data from a motion tracker sensor */

/* List of triggers that use the mouse parameters 
 */
#define PG_TRIGGERS_MOUSE        (PG_TRIGGER_PNTR_RELATIVE | PG_TRIGGER_UP | PG_TRIGGER_DOWN | PG_TRIGGER_MOVE |\
                                  PG_TRIGGER_DRAG | PG_TRIGGER_PNTR_STATUS | PG_TRIGGER_SCROLLWHEEL | PG_TRIGGER_RELEASE |\
                                  PG_TRIGGER_TOUCHSCREEN | PG_TRIGGER_TS_CALIBRATE)

/* list of triggers that use keyboard parameters
 */
#define PG_TRIGGERS_KEY          (PG_TRIGGER_KEYUP | PG_TRIGGER_KEYDOWN | PG_TRIGGER_CHAR | PG_TRIGGER_KEY_START |\
                                  PG_TRIGGER_KEY )

/* Body part flags used with PG_TRIGGER_MOTIONTRACKER */
#define PG_BODYPART_LEFT_HAND      1
#define PG_BODYPART_RIGHT_HAND     2
#define PG_BODYPART_EITHER_HAND    3


/* Note on PG_TRIGGER_RELEASE:  This is when the mouse was pressed inside
   the widget, then released elsewhere.  */


/* More flags used in PicoGUI's keyboard triggers to indicate the circumstances
 * under which a key event is received.
 */
#define PG_KF_FOCUSED           (1<<0)  /* The widget receiving the event is focused */
#define PG_KF_ALWAYS            (1<<3)  /* This is always on */
#define PG_KF_APP_TOPMOST       (1<<4)  /* Set if the widget's app is top in "z-order" */

/******************** pgcolor flags */

#define PGCF_TEXT_ASCII       (0x20 << 24) /* The color is encoded as 0x20BBFFCC, for the
					    * background color, foreground color, and ascii
					    * character. This is only used by the ncurses
					    * driver.
					    */

#define PGCF_TEXT_ACS         (0x40 << 24) /* This is encoded like PGCF_NCURSES_ASCII,
					    * except that an ACS code for special characters
					    * is used instead of a normal ASCII code.
					    */

#define PGCF_ALPHA            (0x80 << 24) /* This indicates an RGB color with an alpha channel,
					    * in the format 0xAARRGGBB. Note that the alpha channel
					    * loses one bit due to this flag, so only 7 bits of alpha
					    * are used.
					    * The corresponding hwrcolor has the same format, except that
					    * the RGB color is in premul format, i.e. each
					    * color has already been multiplied by the alpha channel
					    * then divided by 128.
					    */  

#define PGCF_MASK             (0xFF000000) /* Mask of all color flags */

/******************** pgcolors */

/* Standard HTML colors */
#define PGC_BLACK     0x000000
#define PGC_GREEN     0x008000
#define PGC_SILVER    0xC0C0C0
#define PGC_LIME      0x00FF00
#define PGC_GRAY      0x808080
#define PGC_OLIVE     0x808000
#define PGC_WHITE     0xFFFFFF
#define PGC_YELLOW    0xFFFF00
#define PGC_MAROON    0x800000
#define PGC_NAVY      0x000080
#define PGC_RED       0xFF0000
#define PGC_BLUE      0x0000FF
#define PGC_PURPLE    0x800080
#define PGC_TEAL      0x008080
#define PGC_FUCHSIA   0xFF00FF
#define PGC_AQUA      0x00FFFF


/******************** Global resources */

/*!
 * \defgroup res Global resources
 *
 * The PicoGUI server contains several objects allocated globally, that
 * any client may have limited access to.
 *
 * \{
 */

#define PGRES_DEFAULT_FONT              0
#define PGRES_STRING_OK                 1
#define PGRES_STRING_CANCEL             2
#define PGRES_STRING_YES                3
#define PGRES_STRING_NO                 4
#define PGRES_STRING_SEGFAULT           5
#define PGRES_STRING_MATHERR            6
#define PGRES_STRING_PGUIERR            7
#define PGRES_STRING_PGUIWARN           8
#define PGRES_STRING_PGUIERRDLG         9
#define PGRES_STRING_PGUICOMPAT         10
#define PGRES_DEFAULT_TEXTCOLORS        11
#define PGRES_INFILTER_TOUCHSCREEN      12
#define PGRES_INFILTER_KEY_PREPROCESS   13   /* The _PREPROCESS filters convert to
					      * logical coordinates and do any extra
					      * preprocessing before dispatch
					      */
#define PGRES_INFILTER_PNTR_PREPROCESS  14
#define PGRES_INFILTER_MAGIC            15
#define PGRES_INFILTER_KEY_DISPATCH     16   /* The _DISPATCH filters only send events
					      * to the widgets. Any unused events
					      * pass through them.
					      */
#define PGRES_INFILTER_PNTR_DISPATCH    17
#define PGRES_DEFAULT_CURSORBITMAP      18
#define PGRES_DEFAULT_CURSORBITMASK     19
#define PGRES_BACKGROUND_WIDGET         20
#define PGRES_INFILTER_HOTSPOT          21   /* Recieves global hotspot keys after
					      * the dispatch, and sends more events
					      * back into the front of the pipe.
					      */
#define PGRES_INFILTER_KEY_ALPHA        22   /* Processes PGKEY_ALPHA */
#define PGRES_INFILTER_PNTR_NORMALIZE   23   /* Runs before PGRES_INFILTER_PNTR_PREPROCESS,
					      * handles PNTR_STATUS et al before the touchscreen
					      * calibrator gets to the events.
					      */

#define PGRES_NUM                       24   /* Total number of resources */

//! \}


//! \}
#endif /* __H_PG_CONSTANTS */
/* The End */
