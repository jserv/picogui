
/*
 * divtree.h - define data structures related to divtree management
 * $Revision: 1.4 $
 * 
 * This file contains structures defining and surrounding the divtree, 
 * PicoGUI's method of organizing screen space.
 * The hierarchy of objects is as follows:
 * 	dtstack - a stack of divtrees, providing layering support for popups
 * 	divtree - a collection of divnodes that devide and define space
 * 	divnode - the fundamental multipurpose node of the divtree
 * 	gropnode - a single graphical operation
 *
 * Micah Dowty <micah@homesoftware.com>
 * 
 * This file is released under the GPL. Please see the file COPYING that
 * came with this distribution.
 */

#ifndef __DIVTREE_H
#define __DIVTREE_H

#include <video.h>
#include <handle.h>
#include <g_error.h>

struct widget;
struct dtstack;
struct divtree;
struct divnode;
struct gropnode;

/* This is a stack of divtrees.  The one on top is the currently active
 * tree, and is updated by the renderer.  Trees below this are grayed out
 * to indicate that they do not have focus.  The tree on the bottom, root,
 * covers the whole screen, and has the toolbars and app space.
 */
struct dtstack {
  struct divtree *top;  /* The top of the stack, currently active tree */
  struct divtree *root;  /* The bottom of the stack, root tree */
};

struct divtree {
  struct divnode *head;
  unsigned short int flags;
  int wnum;             /* The ID number of the last numbered widget */
  struct divtree *next;
};

/* Flags used in divtree.flags */
#define DIVTREE_NEED_RECALC	  (1<<0)
#define DIVTREE_NEED_REDRAW	  (1<<1)
#define DIVTREE_ALL_REDRAW	  (1<<2)

/* Alignment types */
typedef short int alignt;
#define A_CENTER   0
#define A_TOP      1
#define A_LEFT     2
#define A_BOTTOM   3
#define A_RIGHT    4
#define A_NW       5
#define A_SW       6
#define A_NE       7
#define A_SE       8
#define A_ALL      9
#define AMAX       9   /* For error checking the range of the align */

/* Parameter structures for gropnodes and divnodes */
union div_param {
  /* Stuff for drawing text */
  struct {          /* This structure is used when on_recalc == &text */
    alignt align;
    devcolort fill;
    int transparent;

    handle string;
    handle fd;
    devcolort col;
  } text;

  /* Bitmaps */
  struct {
    alignt align;
    devcolort fill;
    int transparent;

    handle bitmap;
    handle mask;
    int lgop;
  } bitmap;

  /* colors */
  devcolort c;

  /* generic */
  int i;
};

union grop_param {
  /* Stuff for drawing text */
  struct {          /* This structure is used when on_recalc == &text */
    handle string,fd;
    devcolort col;
  } text;

  /* Bitmaps */
  struct {
    handle bitmap;
    int lgop;
  } bitmap;

  /* colors */
  devcolort c;
};

struct divnode {
   /* The 'secondary pointer', contains the space that was 
    * divided off, usually. This is rendered second. */
   struct divnode *div;
   /* The 'main pointer', contains the left-over space. This is rendered
    * first. */
   struct divnode *next;
   
   unsigned short int flags;
   int split;   /* Depending on flags, the pixels or percent to split at */
   
   /* This function is called after the node is recalculated, so for example
    * a widget can recreate its groplist with new dimensions */
   void (*on_recalc)(struct divnode *self);
   
   /* Extra info that may be needed by on_recalc */
   union div_param param;
   
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
   int x,y;
   
   /* Width and height, of course */
   int w,h;
   
   /* Coordinates to translate the grop's by, for scrolling */
   int tx,ty;
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
#define DIVNODE_PROPAGATE_RECALC (1<<9)

/* Values for the 'side' parameter */

#define S_LEFT    DIVNODE_SPLIT_LEFT
#define S_RIGHT   DIVNODE_SPLIT_RIGHT
#define S_TOP     DIVNODE_SPLIT_TOP
#define S_BOTTOM  DIVNODE_SPLIT_BOTTOM
typedef unsigned short int sidet;

/* And the divnode's flags with this to clear the split type */
#define SIDEMASK  (~(DIVNODE_SPLIT_TOP|DIVNODE_SPLIT_BOTTOM|  \
		     DIVNODE_SPLIT_LEFT|DIVNODE_SPLIT_RIGHT|  \
		     DIVNODE_SPLIT_BORDER|DIVNODE_SPLIT_CENTER))

struct gropnode {
  int type;
  struct gropnode *next;   
  int x,y;  /* Not absolute- transformed by values in the divnode */
  
  int w,h;
  
  /* The rest of the parameters aren't always used, and depend on type */
  union grop_param param;
};
 
/* Possible gropnode types */
#define GROP_NULL	0	/* Doesn't do anything - for temporarily
				 * turning something off */
#define GROP_PIXEL	1	
#define GROP_LINE	2
#define GROP_RECT	3
#define GROP_FRAME      4
#define GROP_SLAB       5
#define GROP_BAR        6
#define GROP_DIM        7
#define GROP_TEXT       8
#define GROP_BITMAP     9

/***************** grop functions */

void grop_render(struct divnode *div);
void grop_addnode(struct gropnode **headpp,struct gropnode *node);
void grop_free(struct gropnode **headpp);
g_error grop_pixel(struct gropnode **headpp,
		   int x, int y, devcolort c);
g_error grop_line(struct gropnode **headpp,
		  int x, int y, int w, int h, devcolort c);
g_error grop_rect(struct gropnode **headpp,
		  int x, int y, int w, int h, devcolort c);
g_error grop_dim(struct gropnode **headpp);
g_error grop_frame(struct gropnode **headpp,
		   int x, int y, int w, int h, devcolort c);
g_error grop_slab(struct gropnode **headpp,
		  int x, int y, int l, devcolort c);
g_error grop_bar(struct gropnode **headpp,
		 int x, int y, int l, devcolort c);
g_error grop_text(struct gropnode **headpp,
		  int x, int y, handle fd, devcolort col, handle str);
g_error grop_bitmap(struct gropnode **headpp,
		    int x, int y, int w, int h, handle b, int lgop);

void align(struct divnode *d,alignt align,int *w,int *h,int *x,int *y);

/***************** divnode functions */

void divnode_recalc(struct divnode *n);
void divnode_redraw(struct divnode *n,int all);
g_error newdiv(struct divnode **p,struct widget *owner);
void r_divnode_free(struct divnode *n);

/***************** divtree management */

g_error divtree_new(struct divtree **dt);
void divtree_free(struct divtree *dt);
void update(struct dtstack *s);
void r_dtupdate(struct divtree *dt);
g_error dts_new(struct dtstack **p);
void dts_free(struct dtstack *dts);
g_error dts_push(struct dtstack *dts);
void dts_pop(struct dtstack *dts);

#endif /* __DIVTREE_H */

/* The End */







