/* $Id: divtree.h,v 1.8 2000/12/31 16:52:32 micahjd Exp $
 *
 * divtree.h - define data structures related to divtree management
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

#ifndef __DIVTREE_H
#define __DIVTREE_H

#include <picogui/constants.h>

#include <pgserver/handle.h>
#include <pgserver/g_error.h>
#include <pgserver/video.h>

struct widget;
struct dtstack;
struct divtree;
struct divnode;
struct gropnode;
struct gropctxt;

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

struct divtree {
  struct divnode *head;
  unsigned short int flags;
  struct widget *hkwidgets; /* Pointer to head widget in hotkey list*/
  int wnum;             /* The ID number of the last numbered widget */
  struct divtree *next;
};

/* Flags used in divtree.flags */
#define DIVTREE_NEED_RECALC	  (1<<0)
#define DIVTREE_NEED_REDRAW	  (1<<1)
#define DIVTREE_ALL_REDRAW	  (1<<2)

typedef short int alignt;

struct divnode {
  /* The 'secondary pointer', contains the space that was 
   * divided off, usually. This is rendered second. */
  struct divnode *div;
  /* The 'main pointer', contains the left-over space. This is rendered
   * first. */
  struct divnode *next;
  
  /* When the widget is resized or changes state, this is used to rebuild the
     groplist. It defines the appearance of the widget. */
  void (*build)(struct gropctxt *c,unsigned short state,struct widget *self);
  
  /* If this pointer is not null, the groplist is rendered to this divnode */
  struct gropnode *grop;
  
  /* Widget that owns it, used for updating widget 'where' pointers
     on deleting a widget */
  struct widget *owner;
  
  /* Absolute coordinates of the node on the screen.  This is usually
   * calculated by various flags using split, but if no such flags are used
   * the coordinates can be specified absolutely, however this is normally
   * not reccomended (but could be useful for constructs such as popup menus)
   */
  short int x,y;
  
  /* Width and height, of course */
  short int w,h;
  
  /* Coordinates to translate the grop's by, for scrolling */
  short int tx,ty;
  /* Scrolling coordinates as of last redraw */
  short int otx,oty;

  unsigned short int flags;
  short int split;   /* Depending on flags, the pixels or percent to split at */

  /* The divnode's state - indicates which theme object to get parameters from */
  unsigned short state;
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
#define DIVNODE_PROPAGATE_RECALC (1<<9) /* recalc spreads through next also */
#define DIVNODE_SPLIT_IGNORE   (1<<10) /* Don't bother the nodes' positions */
#define DIVNODE_SPLIT_EXPAND   (1<<11) /* Expand div to all available space */
#define DIVNODE_PROPAGATE_REDRAW (1<<12) /* redraw spreads through next also*/
#define DIVNODE_SCROLL_ONLY     (1<<13)  /* Only tx/ty changed */
#define DIVNODE_INCREMENTAL     (1<<14)  /* Only incremental redraw */

/* Side value macros and stuff */
typedef unsigned short int sidet;
#define VALID_SIDE(x) (x==PG_S_LEFT || x==PG_S_RIGHT || x==PG_S_TOP || x==PG_S_BOTTOM \
		       || x==PG_S_ALL)

/* And the divnode's flags with this to clear the split type */
#define SIDEMASK  (~(DIVNODE_SPLIT_TOP|DIVNODE_SPLIT_BOTTOM|    \
		     DIVNODE_SPLIT_LEFT|DIVNODE_SPLIT_RIGHT|    \
		     DIVNODE_SPLIT_BORDER|DIVNODE_SPLIT_CENTER| \
                     DIVNODE_SPLIT_IGNORE|DIVNODE_SPLIT_EXPAND))

/* The maximum number of parameters a gropnode could need (client
   doesn't depend on this number. only for memory allocation purposes!)
*/
#define NUMGROPPARAMS    5

struct gropnode {
  short int type,flags,x,y,w,h;
  struct gropnode *next;   
  unsigned long param[NUMGROPPARAMS];
};

/* These are applied to new gropnodes */
extern short int defaultgropflags;

/***************** grop contexts */

/* The grop context stores the information necessary for one 'session' of
   gropnode building or updating */

struct gropctxt {
  struct gropnode **headpp;   /* Head of groplist */
  struct gropnode *current;   /* Current position */
  unsigned short n;           /* Numerical position in gropnode list */
  int x,y,w,h;                /* Current coordinates */
};

/* Set up a grop context for rendering to a divnode */
void gropctxt_init(struct gropctxt *ctx, struct divnode *div);

/* Add a new gropnode to the context. Caller fills in
   all the grop's parameters (ctx->current->foo) afterwards. */
g_error addgrop(struct gropctxt *ctx, int type,int x,int y,int w,int h);

/***************** grop functions */

void grop_render(struct divnode *div);
void grop_addnode(struct gropnode **headpp,struct gropnode *node);
void grop_free(struct gropnode **headpp);

void align(struct gropctxt *d,alignt align,int *w,int *h,int *x,int *y);

/***************** divnode functions */

void divnode_recalc(struct divnode *n);
void divnode_redraw(struct divnode *n,int all);
g_error newdiv(struct divnode **p,struct widget *owner);
void r_divnode_free(struct divnode *n);

/* Little helper to rotate a side constant 90 degrees counterclockwise */
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
void dts_pop(void);

/* Update the screen, starting at 'subtree'
   Sprites are hidden first, if needed.
   If show is nonzero, sprites are shown and
   the hardware is updated, completing the process.

   The old update function is equivalent to update(dts->top,1);
*/
void update(struct divnode *subtree,int show);

#endif /* __DIVTREE_H */

/* The End */







