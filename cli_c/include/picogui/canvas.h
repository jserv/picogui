/* $Id: canvas.h,v 1.12 2001/04/12 02:38:34 micahjd Exp $
 *
 * picogui/canvas.h - This defines the commands sent from the client to a
 *                    canvas widget (Via RQH_WRITETO)
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000,2001 Micah Dowty <micahjd@users.sourceforge.net>
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

#ifndef _H_PG_CANVAS
#define _H_PG_CANVAS

/* Drawing can happen in four ways:
 * 
 * 1. When the widget is rebuilt - this usually happens when it is resized,
 *    and gives the app a chance to recalculate size-dependant things.
 *    The groplist is automatically cleared and the PG_WE_BUILD event is sent.
 * 
 * 2. Individual gropnodes can be modified and marked as such.
 *    This is useful when you have many gropnodes, for example pieces on
 *    a game board, that will be changed individually. This uses a
 *    'pseudo-incremental' gropnode. The app changes necessary parameters,
 *    then marks the grops that need redrawing as 'pseudo-incremental'
 *    They are drawn on full or incremental updates and the flag is always
 *    cleared afterwards.
 * 
 * 3. The incremental gropnode is a bit more specialized - it allows the app
 *    to keep two seperate groups of grops in the list. Normal gropnodes are
 *    redrawn only on full updates. Incremental updates are drawn _only_
 *    on incremental updates. Think of a tetris-like game for example:
 *    when a piece scrolls down the screen, the piece itself could be marked
 *    as pseudo-incremental. The game would also need to redraw the background
 *    where the piece was, but if it redrew the entire background it would
 *    obscure the other pieces. The solution is to create a seperate
 *    incremental grop set to the previous location of the moving piece.
 *    (Note though that this behavior could also be accomplished with sprites)
 * 
 * 4. Scrolling. Grops marked with the PG_GROPF_TRANSLATE flag will translate
 *    by the widget's horizontal and vertical scrolling amounts before
 *    rendering. If only the scrolling amounts are changed, a scroll-only
 *    redraw can be set. This uses a screen-to-screen blit to shift
 *    most of the screen and only redraws the sliver along the edge.
 *
 */

/************************ Canvas commands */

/* Removes all gropnodes from the list.
 * This is done automatically immediately before a PG_WE_BUILD is sent
 * so usually it is not necessary to call directly.
 * (please modify existing gropnodes if possible, instead of fully rebuilding
 * each time!)
 * 
 * Parameters: none
 */
#define PGCANVAS_NUKE        1

/* Adds a new gropnode to the end of the list.
 * This is usually used in PG_WE_BUILD, though it is possible to use anywhere.
 * It sets the 'current gropnode' to this newly created node.
 * 
 * Parameters:
 *  1. Type (a PG_GROP_* constant)
 *  2. X position (relative to widget)
 *  3. Y position (relative to widget)
 *  4. width
 *  5. height
 *
 * Note on width and height: This is the standard way of measuring size in
 * PicoGUI. It is applicable in most cases, but there are a few notable
 * exceptions:
 * 
 *  1. The height of a slab should be 1
 *  2. The width of a bar should be 1
 *  3. Width and height of text is ignored (but avoid values <= 0)
 *  4. In a line, width is interpreted as x2 - x1, height as y2 - y1. This
 *     means that a vertical line has zero width and it is alright for width
 *     or height to be negative.
 * 
 */
#define PGCANVAS_GROP        2

/* Renders a theme fillstyle. This is normally not needed, as canvases need
 * not depend on themes, but it may be useful to render a button background
 * into your canvas, for example.
 * 
 * There are a few things to watch out for if you use this:
 *  1. Allthough the fillstyle's coordinates are translated, they are not
 *     clipped. It is possible but unlikely that it will draw all over your
 *     canvas.
 *  2. The fillstyle will create an unknown number of gropnodes. You will not
 *     be able to rely on your gropnode indices increasing steadily, and you
 *     will need to store the gropnode numbers of important nodes.
 *  3. Some fillstyles may be designed only to work at certain sizes. The
 *     Aqua theme's buttons, for example, only render correctly at the
 *     button's intended height. More generic fillstyles like box or background
 *     should work at any size.
 *  4. It will set the 'current gropnode' pointer to the last gropnode
 *     in the fill, but you shouldn't rely on anything special being there.
 * 
 * Parameters:
 *  1. Theme object (a PGTH_O_* constant)
 *  2. Theme property (a PGTH_P_* constant, but almost always PGTH_P_BGFILL
 *     or PGTH_P_OVERLAY. Invalid fillstyles will render as a white
 *     rectangle with black border)
 *  3. X position relative to widget
 *  4. Y position relative to widget
 *  5. Width
 *  6. Height
 * 
 */
#define PGCANVAS_EXECFILL    3

/* Sets the 'current grop' pointer to the grop specified by the index.
 * 
 * Parameters:
 *  1. Zero-based gropnode index
 */
#define PGCANVAS_FINDGROP    4

/* Sets the parameters of the current gropnode.
 * These parameters are gropnode dependant, and not yet well documented.
 * As a last resort you can look in grop.c
 * Simple grops like line, recatngle, slab, etc. take a hwrcolor as their
 * first parameter.
 * 
 * Parameters:
 *  *. Gropnode params
 */
#define PGCANVAS_SETGROP     5

/* Sets the size and position of the current gropnode
 * 
 * Parameters:
 *  1. X position (relative to widget)
 *  2. Y position (relative to widget)
 *  3. width
 *  4. height
 */
#define PGCANVAS_MOVEGROP    6

/* Changes the type of the current gropnode.
 * You can use PG_GROP_NULL to disable it
 * 
 * Parameters:
 *  1. Grop type (a PG_GROP_* constant)
 */
#define PGCANVAS_MUTATEGROP  7

/* According to the bitmask, it converts the parameters
 * of the current gropnode from pgcolor to hwrcolor.
 * In a rectangle, for example, you must use SETGROP to send
 * the pgcolor as the first param, then call COLORCONV with
 * a bitmask of '1' to convert the first parameter to a hwrcolor
 * 
 * Parameters:
 *  1. Bitmask, LSB is the first param
 */
#define PGCANVAS_COLORCONV   8

/* Sets the flags on the current grop. This is for marking grops as
 * incremental, pseudoincremental, translated, etc.
 * 
 * Parameters:
 *  1. Zero or more PG_GROPF_* constants or'ed together
 */
#define PGCANVAS_GROPFLAGS   9

/* Sets the proper flags for a full redraw. Note that this doesn't
 * actually draw anything, you must call pgSubUpdate or its equivalent
 * to redraw the portion of the screen.
 * 
 * Parameters: none
 */
#define PGCANVAS_REDRAW      10

/* Sets an incremental redraw. Normal grops are not drawn, incremental grops
 * are drawn, pseudo-incremental grops are drawn but the flag is then reset.
 * 
 * Parameters: none
 */
#define PGCANVAS_INCREMENTAL 11

/* Scrolls the widget to the indicated position and sets a scroll-only redraw.
 * The screen is always blitted but the grops themselves only move if they
 * have PG_GROPF_TRANSLATE
 * 
 * Parameters:
 *  1. X translation
 *  2. Y translation
 */
#define PGCANVAS_SCROLL      12

#endif /* __H_PG_CANVAS */
/* The End */

