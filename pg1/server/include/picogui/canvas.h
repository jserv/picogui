/* $Id$
 *
 * picogui/canvas.h - This defines the commands sent from the client to a
 *                    canvas widget (Via RQH_WRITECMD)
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

/*! 
 * \file canvas.h
 * \brief Canvas widget low-level commands
 * 
 * Normally, PGFX should be used to draw in a Canvas widget.
 * There are some features that are not accessable through PGFX,
 * so it may be desirable to access the Canvas widget directly.
 * The constants in this file are command codes to use with
 * pgWriteCmd.
 * 
 * Usually this file does not need to be included
 * separately, it is included with <tt>\#include <picogui.h></tt>
 *
 * The Canvas widget provides an interface between clients and a list
 * of gropnodes. Gropnodes (short for GRaphical OPeration Nodes) are the
 * fundamental unit of graphics in the PicoGUI server. Each gropnode has
 * a type (a PG_GROP_* constant) indicating what primitive it represents.
 * Most gropnodes also have a position, expressed as (x,y) coordinates,
 * width, and height. Some gropnodes also have parameters, and gropnodes
 * can be given flags (PG_GROPF_* constants) that alter how they are rendered.
 * This interface to the Canvas widget allows the client to create and
 * manipulate lists of gropnodes.
 * 
 * \sa pgWriteCmd, pgNewCanvasContext, PG_WIDGET_CANVAS
 */

#ifndef _H_PG_CANVAS
#define _H_PG_CANVAS

/************************ Canvas commands */

/*! 
 * \brief Removes all gropnodes from the list
 *
 * \param none
 */
#define PGCANVAS_NUKE        1

/*!
 * \brief Add a new gropnode to the end of the list
 * 
 * The 'current gropnode' becomes the newly built node
 * 
 * \param 1. gropnode type (a PG_GROP_* constant)
 * \param x. Unless the gropnode is 'unpositioned' (setcolor and friends) the
 *           next 4 args are the x,y,w,h positioning (all 4 are required!)
 * \param x. Gropnode parameters
 *
 * Note on width and height: This is the standard way of measuring size in
 * PicoGUI. It is applicable in most cases, but there are a few notable
 * exceptions:
 * 
 *  - The height of a slab should be 1
 *  - The width of a bar should be 1
 *  - Width and height of text is ignored (but avoid values <= 0)
 *  - In a line, width is interpreted as x2 - x1, height as y2 - y1. This
 *    means that a vertical line has zero width and it is alright for width
 *    or height to be negative.
 *  - A pixel should have width and height of 1
 * 
 */
#define PGCANVAS_GROP        2

/*!
 * \brief Render a theme fillstyle 
 *
 * This is normally not needed, as canvases need
 * not depend on themes, but it may be useful to render a button background
 * into your canvas, for example.
 * 
 * There are a few things to watch out for if you use this:
 *  - Allthough the fillstyle's coordinates are translated, they are not
 *    clipped. It is possible but unlikely that it will draw all over your
 *    canvas.
 *  - The fillstyle will create an unknown number of gropnodes. You will not
 *    be able to rely on your gropnode indices increasing steadily, and you
 *    will need to store the gropnode numbers of important nodes.
 *  - Some fillstyles may be designed only to work at certain sizes. The
 *    Aqua theme's buttons, for example, only render correctly at the
 *    button's intended height. More generic fillstyles like box or background
 *    should work at any size.
 *  - It will set the 'current gropnode' pointer to the last gropnode
 *    in the fill, but you shouldn't rely on anything special being there.
 * 
 * \param 1. Theme object (a PGTH_O_* constant)
 * \param 2. Theme property (a PGTH_P_* constant, but almost always PGTH_P_BGFILL
 *           or PGTH_P_OVERLAY. Invalid fillstyles will render as a white
 *           rectangle with black border)
 * \param 3. X position relative to widget
 * \param 4. Y position relative to widget
 * \param 5. Width
 * \param 6. Height
 * 
 */
#define PGCANVAS_EXECFILL    3

/*!
 * \brief Set the 'current gropnode'
 * 
 * \param 1. Zero-based gropnode index
 */
#define PGCANVAS_FINDGROP    4

/*!
 * \brief Set the parameters of the current gropnode
 * 
 * These parameters are gropnode dependant, see the documentation
 * for the gropnode type itself. (The PG_GROP_* constant)
 * 
 * \param x. Gropnode params
 */
#define PGCANVAS_SETGROP     5

/*!
 * \brief Set the size and position of the current gropnode
 * 
 * \param 1. X position (relative to widget)
 * \param 2. Y position (relative to widget)
 * \param 3. width
 * \param 4. height
 */
#define PGCANVAS_MOVEGROP    6

/*!
 * \brief Change the type of the current gropnode
 * 
 * Also note that you can use PG_GROP_NULL to disable it
 * 
 * \param 1. Grop type (a PG_GROP_* constant)
 */
#define PGCANVAS_MUTATEGROP  7

/*!
 * \brief Set the default flags given to new gropnodes
 *
 * This is especially good for things
 * like setting the PG_GROPF_TRANSIENT flags to make all new grops behave
 * as if they're rendered immediately and not stored, or turing on the
 * PG_GROPF_TRANSLATE flag to define a scrolled region of the canvas.
 * 
 * \param 1. Zero or more PG_GROPF_* constants or'ed together
 */
#define PGCANVAS_DEFAULTFLAGS   8

/*!
 * \brief Set the flags on the current grop
 *
 * This is for marking grops as
 * incremental, pseudoincremental, translated, transient, etc.
 * 
 * \param 1. Zero or more PG_GROPF_* constants or'ed together
 */
#define PGCANVAS_GROPFLAGS   9

/*!
 * \brief Set the proper flags for a full redraw
 * 
 * Note that this doesn't
 * actually draw anything, you must call pgSubUpdate or its equivalent
 * to redraw the portion of the screen. Incremental gropnodes are not
 * drawn in a full redraw
 * 
 * \param none
 */
#define PGCANVAS_REDRAW      10

/*! 
 * \brief Set an incremental redraw
 *
 * Normal grops are not drawn, incremental grops
 * are drawn, pseudo-incremental grops are drawn but the flag is then reset.
 * 
 * \param none
 */
#define PGCANVAS_INCREMENTAL 11

/*!
 * \brief Scrolls the widget to the indicated position and sets a scroll-only redraw
 * 
 * The screen is always blitted but the grops themselves only move if they
 * have PG_GROPF_TRANSLATE. If there are gropnodes without the PG_GROPF_TRANSLATE
 * flag that are not uniform across the axis that is being scrolled (for example,
 * almost anything other than a rectangle or, in some cases, a gradient) the
 * results are undefined and can be a bit odd.
 * 
 * \param 1. X translation
 * \param 2. Y translation
 */
#define PGCANVAS_SCROLL      12

/*!
 * \brief Set the mapping type used for mouse/touchpad input
 * 
 * Works just like the PG_GROP_SETMAPPING gropnode or pgSetMapping PGFX call, but maps in reverse.
 * 
 * \param 1. x
 * \param 2. y
 * \param 3. w
 * \param 4. h
 * \param 5. mapping type (PG_MAP_*)
 */
#define PGCANVAS_INPUTMAPPING  13

/*!
 * \brief Set the preferred size per mapping unit
 * 
 * When using PG_MAP_SCALE, this is useful to set the
 * preferred size of the virtual grid squares in the canvas.
 * This is especially useful when using a canvas in a dialog
 * box or other container.
 *
 * \param 1. w
 * \param 2. h
 */
#define PGCANVAS_GRIDSIZE      14

/*!
 * \brief Add a sequencec of new gropnode to the end of the list
 * 
 * The 'current gropnode' becomes the last newly built node
 * 
 * The parameters are a concatenation of N instances of the following:
 *
 * \param 1. length of this grop (how many parameters, counting this one)
 * \param 2. gropnode type (a PG_GROP_* constant)
 * \param x. Unless the gropnode is 'unpositioned' (setcolor and friends) the
 *           next 4 args are the x,y,w,h positioning (all 4 are required!)
 * \param x. Gropnode parameters
 *
 * See PGCANVAS_GROP for notes on the parameters.
 *
 */
#define PGCANVAS_GROPSEQ       15

#endif /* __H_PG_CANVAS */
/* The End */

