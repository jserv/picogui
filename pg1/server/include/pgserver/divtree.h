/* $Id$
 *
 * divtree.h - define data structures related to divtree management
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

#ifndef __DIVTREE_H
#define __DIVTREE_H

#include <picogui/constants.h>

#include <pgserver/handle.h>
#include <pgserver/g_error.h>
#include <pgserver/video.h>
#include <pgserver/render.h>

struct widget;
struct dtstack;
struct divtree;
struct divnode;
struct gropnode;
struct gropctxt;

extern struct divtree fakedt;
#define DT_NIL (&fakedt)

/* This is a stack of divtrees.  The one on top is the currently active
 * tree, and is updated by the renderer.  Trees below this are grayed out
 * to indicate that they do not have focus.  The tree on the bottom, root,
 * covers the whole screen, and has the toolbars and app space.
 */
struct dtstack {
  struct divtree *top;  /* The top of the stack, currently active tree */
  struct divtree *root;  /* The bottom of the stack, root tree */
};

/* The One True Stack */
extern struct dtstack *dts;

/* A tree of divnodes, and metadata associated with them */
struct divtree {
  struct divnode *head;           /* Head of the divnode tree */
  unsigned short int flags;       /* DIVTREE_* flags */

  handle focus;                   /* Handle to the focused widget in this divtree */
  struct cursor *hotspot_cursor;  /* Hotspot cursor for this divtree, if it exists */
  
  handle h;                       /* Handle to this divtree */

  /* To support rootless and multihead modes, each divtree has its own rendering information */
  hwrbitmap display;
  struct pgrect update_rect;

  struct divtree *next;
};

/* Flags used in divtree.flags */
#define DIVTREE_NEED_RECALC	  (1<<0)
#define DIVTREE_NEED_REDRAW	  (1<<1)
#define DIVTREE_ALL_REDRAW	  (1<<2)
#define DIVTREE_ALL_NONTOOLBAR_REDRAW (1<<3)
#define DIVTREE_CLIP_POPUP        (1<<4)
#define DIVTREE_NEED_RESIZE       (1<<5)

struct divnode {
  /* The 'secondary pointer', contains the space that was 
   * divided off, usually. This is rendered second. */
  struct divnode *div;
  /* The 'main pointer', contains the left-over space. This is rendered
   * first. */
  struct divnode *next;
  
  /* This is used by the DIVNODE_DIVSCROLL flag */
  struct divnode *divscroll;

  /* When the widget is resized or changes state, this is used to rebuild the
     groplist. It defines the appearance of the widget. */
  void (*build)(struct gropctxt *c,unsigned short state,struct widget *self);
  
  /* If this pointer is not null, the groplist is rendered to this divnode */
  struct gropnode *grop;
  
  /* Widget that owns it, used for updating widget 'where' pointers
   * on deleting a widget, and to determine the container when using
   * DIVNODE_UNIT_CNTFRACT */
  struct widget *owner;
   
  u32 flags;
   
  /* Absolute coordinates of the node on the screen.  This is usually
   * calculated by various flags using split, but if no such flags are used
   * the coordinates can be specified absolutely, however this is normally
   * not reccomended (but could be useful for constructs such as popup menus)
   */
  struct pgrect r;

  /* Calculated size. Normally the same as the size above, but:
   *  - The position will be different if DIVNODE_DIVSCROLL is on
   *  - The size will be different if one of the DIVNODE_EXTEND_* flags
   *    are turned on.
   */
  struct pgrect calc;

  /* The preferred width and height as calculated by the widget itself.
   * This is taken as a minimum size when the divnode contains other divnodes,
   * because the 'actual' preferred size is calculated as max(pw,cw),max(ph,ch)
   */
  struct sizepair preferred;
	
  /* Child width and height. As long as DIVNODE_SIZE_RECURSIVE is set,
   * this is calculated from the combined size of the child divnodes */
  struct sizepair child;
	
  /* Coordinates to translate the grop's by, for scrolling */
  struct pgpair translation;

  /* Scrolling coordinates as of last redraw */
  struct pgpair old_translation;

  s16 split;   /* Depending on flags, the pixels or percent to split at */

  /* The divnode's state - indicates which theme object to get parameters from */
  u16 state;
};
   
/* flags used in divnode.flags */
#define DIVNODE_NEED_RECALC	(1<<0)
#define DIVNODE_NEED_REDRAW	(1<<1)
#define DIVNODE_UNIT_PERCENT    (1<<2)   /* Default is pixels */
#define DIVNODE_SPLIT_TOP	(1<<3)
#define DIVNODE_SPLIT_BOTTOM    (1<<4)
#define DIVNODE_SPLIT_LEFT      (1<<5)
#define DIVNODE_SPLIT_RIGHT	(1<<6)
#define DIVNODE_SPLIT_BORDER	(1<<7) /* Shave 'split' pixels off each side */
#define DIVNODE_SPLIT_CENTER    (1<<8) /* Keep div's w and h, center it */
#define DIVNODE_NEED_REBUILD    (1<<9) /* Run div_rebuild() on the next update */
#define DIVNODE_SPLIT_IGNORE   (1<<10) /* Don't bother the nodes' positions */
#define DIVNODE_SPLIT_EXPAND   (1<<11) /* Expand div to all available space */
#define DIVNODE_PROPAGATE_REDRAW (1<<12) /* redraw spreads through next also*/
#define DIVNODE_SCROLL_ONLY     (1<<13)  /* Only tx/ty changed */
#define DIVNODE_INCREMENTAL     (1<<14)  /* Only incremental redraw */
#define DIVNODE_UNIT_CNTFRACT   (1<<15)  /* This is used to build grids of
					  * self-resizing widgets. The size
					  * is specified as a fraction of the
					  * container's size. The fraction
					  * is packed with numerator in high
					  * byte and denominator in low byte */
#define DIVNODE_GROPLIST_DIRTY  (1<<16)  /* Set whenever the order or
					  * length of the groplist is modified,
					  * lets the owner widget
					  * know when it needs to update
					  * pointers. It's the widget's
					  * responsibility to clear the flag
					  * if necessary */
#define DIVNODE_SIZE_RECURSIVE  (1<<17)  /* The preferred size is calculated
					  * from contained divnodes
					  * recursively, container's px,py is
					  * used as it's minimum size */
#define DIVNODE_SIZE_AUTOSPLIT  (1<<18)  /* When applicable, calculate 'split'
					  * automatically based on the
					  * preferred size */
#define DIVNODE_SPLIT_POPUP     (1<<19)  /* The 'div' child's size is interpreted
					  * as a popup box size (with optional
					  * centering, "at mouse cursor", and
					  * automatic sizing) then the absolute
					  * position and size are stored back
					  * to the divnode and this flag is
					  * cleared. Should usually be combined
					  * with DIVNODE_SPLIT_IGNORE */
#define DIVNODE_HOTSPOT         (1<<20)  /* This divnode is a hotspot that can be
					  * selected with the arrow keys. All
					  * divnodes with this flag get sorted,
					  * then the owner widget's flags are set
					  * correctly to allow movement
					  * between them */
#define DIVNODE_POPUP_NONTOOLBAR (1<<21) /* For popups, clip to the nontoolbar
					  * area and allow events and drawing from
					  * toolbars while the popup is onscreen.
					  * This flag is given to the actual popup
					  * widget, the 'div' child of the widget
					  * given DIVNODE_SPLIT_POPUP */
#define DIVNODE_RAW_BUILD        (1<<22) /* Don't automatically clear the groplist,
					  * initialize the grop context, and set
					  * redraw flags when doing a div_rebuild()
					  */
#define DIVNODE_EXTEND_WIDTH     (1<<23) /* Use the calculated width or
					  * the preferred width, whichever's
					  * larger.
					  */
#define DIVNODE_EXTEND_HEIGHT    (1<<24) /* Use the calculated height or
					  * the preferred height, whichever's
					  * larger.
					  */
#define DIVNODE_DIVSCROLL        (1<<25) /* If 'divscroll' is NULL, set it to ourselves.
					  * Give a copy of our 'divscroll' pointer to our
					  * children, and set DIVNODE_DIVSCROLL_CHILD
					  * flags for children. Translate by divscroll->tx,
					  * divscroll->ty and clip with divscroll->calcw
					  * and divscroll->calch.
					  *
					  * Along with DIVNODE_EXTEND_HEIGHT or _WIDTH this
					  * implements divnode-level scrollin
					  */
#define DIVNODE_PROPAGATE_SCROLL (1<<26) /* This is used in divnode_redraw to give all children
					  * of a scrolled node the DIVNODE_SCROLL_ONLY flag
					  * when the scrolled node gets it 
					  */
#define DIVNODE_UNDERCONSTRUCTION (1<<29) /* This flag prevents a divnode from being calculated
                                           * or rendered. The 'next' child inherits this div's
					   * position, and the 'div' child is not calculated or
					   * rendered. This flag is cleared in all widgets owned
					   * by an app when that app waits for an event.
					   */ 
#define DIVNODE_FORCE_CHILD_RECALC (1<<30) /* Normally child divnodes are only recalc'ed if 
					    * the size changes. This forces it.
					    */
#define DIVNODE_ANIMATED          (1<<31)  /* this divnode is to be periodically rebuilt because
					    * it's groplist depends on time.
					    */

/* Side value macros and stuff */
typedef unsigned short int sidet;
#define VALID_SIDE(x) (x==PG_S_LEFT || x==PG_S_RIGHT || x==PG_S_TOP || x==PG_S_BOTTOM \
		       || x==PG_S_ALL)

/* And the divnode's flags with this to clear the split type */
#define SIDEMASK  (~(DIVNODE_SPLIT_TOP|DIVNODE_SPLIT_BOTTOM|    \
		     DIVNODE_SPLIT_LEFT|DIVNODE_SPLIT_RIGHT|    \
		     DIVNODE_SPLIT_BORDER|DIVNODE_SPLIT_CENTER| \
                     DIVNODE_SPLIT_IGNORE|DIVNODE_SPLIT_EXPAND))

/***************** divnode functions */

/* Recursively recalculate divnode positions. This is the core of the
 * layout engine. The pn parameter should be a pointer to the parent's
 * pointer to this divnode. This is important in case this divnode is moved
 * to another location in the tree. The parent parameter optionally points
 * to this divnode's parent, or NULL.
 */
void divnode_recalc(struct divnode **pn, struct divnode *parent);

/* Split a divnode into two rectangles according to its flags */
void divnode_split(struct divnode *n,struct pgrect *div,
		   struct pgrect *next);
void divnode_redraw(struct divnode *n,int all);
g_error newdiv(struct divnode **p,struct widget *owner);
void r_divnode_free(struct divnode *n);
void r_set_nextline(struct divnode *n, struct divnode *nl);

/* Given a starting point and a node, this finds the 'div' branch the node
 * is on. This is used in word wrapping to figure out what line a
 * divnode is on.
 */
struct divnode *divnode_findbranch(struct divnode *tree, struct divnode *dest);

/* Find a pointer to the supplied divnode pointer in the supplied tree.
 * This is useful for deleting divnodes.
 */
struct divnode **divnode_findpointer(struct divnode *tree,
				     struct divnode *dest);

/* Find a divnode's parent */
struct divnode *divnode_findparent(struct divnode *tree,
				   struct divnode *dest);

/* Little helper to rotate a side constant 90 degrees counterclockwise */

/* Remove unnecessary scrolling information from the sub-divtree */
void r_div_unscroll(struct divnode *div);
int rotate_side(int s);
/* Rotate them 90 degrees counterclockwise, then mirror
   across the horizontal axis */
int mangle_align(int al);

/***************** divtree management */

g_error divtree_new(struct divtree **dt);
void divtree_free(struct divtree *dt);
void r_dtupdate(struct divtree *dt);
g_error dts_new(void);
void dts_free(void);
g_error dts_push(void);
void dts_pop(struct divtree *dt);

/* Update the screen, starting at 'subtree'
   Sprites are hidden first, if needed.
   If show is nonzero, sprites are shown and
   the hardware is updated, completing the process.

   The old update function is equivalent to update(dts->top,1);
*/
void update(struct divnode *subtree,int show);

/* Performs the resize/recalc steps if necessary */
void divtree_size_and_calc(struct divtree *dt);

/* This function returns nonzero if there is more than one divtree layer,
   and all layers except for the root divtree has DIVNODE_POPUP_NONTOOLBAR */
int popup_toolbar_passthrough(void);

/* Returns nonzero if the specified divnode is within a toolbar root widget */
int divnode_in_toolbar(struct divnode *div);

/* Turn off the DIVNODE_UNDERCONSTRUCTION flags for all divnodes owned by this client
 */
void activate_client_divnodes(int client);

/***************** smart resizing */

/* Calculate a new split value for the specified divnode
 * based on the pw,ph,cw, and ch values of the node's 'div' child */
void divresize_split(struct divnode *div);

/* Recursively recalculate cw and ch for the given divnode */
void divresize_recursive(struct divnode *div); 

#endif /* __DIVTREE_H */

/* The End */







