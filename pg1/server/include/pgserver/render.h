/* $Id$
 *
 * render.h - data structures and functions for rendering and manipulating
 *            gropnodes (Graphics Operation nodes)
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

#ifndef __RENDER_H
#define __RENDER_H

#include <pgserver/handle.h>
#include <pgserver/g_error.h>
#include <pgserver/video.h>
#include <pgserver/font.h>

/* If nonzero, this is the ID of the client that has exclusive display
 * access. It is allowed to render directly to vid->display, and normal
 * rendering should be disabled.
 */
extern int display_owner;

/* this can be turned on to disable rendering when pgserver is on a
 * different virtual terminal, or it's minimized.
 */
extern int disable_output;

/* The maximum number of parameters a gropnode could need (client
 * doesn't depend on this number. only for memory allocation purposes!)
 *
 * The maximum number of gropnode params should be kept as small as possible,
 * because all gropnodes are allocated to the same size for memory management
 * purposes.
 */
#define NUMGROPPARAMS    3    /* NOTE: This is also currently limited by
			       * bitfield space in the PG_GROP_* constants.
			       * 3 should be enough for almost anything, if
			       * more space is really neaded the primitive
			       * should be split up somehow. */

struct gropnode {
   u32 param[NUMGROPPARAMS];
   u16 type,flags;
   struct pgrect r;
   struct gropnode *next;   
};

/* Structure to hold all state info while rendering one groplist */
struct groprender {
   struct pgrect output_rect; /* Size of output device (divnode, etc) */
   struct pgrect orig;        /* Original rect of current gropnode */
   struct pgpair translation; /* Applied only to grops with PG_GROPF_TRANSLATE */
   struct pgpair scroll;      /* Delta translation from last redraw */
   struct pgrect csrc;        /* Additional src_x,src_y offsets from clipping */
   hwrbitmap output;        /* Bitmap to render to */
   struct pgquad orig_clip;   /* Original clipping rectangle, before any
			     * PG_GROP_SETCLIP nodes modify it */
  
   /* Params that can be set with nonvisual gropnodes */
   struct pgrect offset, src, map;
   struct pgquad clip;
   u8 maptype;
   hwrcolor color;     /* Used for all primitives */
   s16 lgop;
   s16 angle;
   handle hfont;
};

/***************** grop contexts */

/* The grop context stores the information necessary for one 'session' of
 * gropnode building or updating. The groplist's structure may change
 * during rendering, so this context can only be valid for a short time!
 */

struct gropctxt {
  struct gropnode **headpp;   /* Head of groplist */
  struct gropnode *current;   /* Current position */
  u32 n;                      /* Numerical position in gropnode list */
  struct pgrect r;              /* Current coordinates */
  struct divnode *owner;      /* Optional */
  /* These are applied to new gropnodes */
  u16 defaultgropflags;
};

/* Set up a grop context for rendering to a divnode */
void gropctxt_init(struct gropctxt *ctx, struct divnode *div);

/* Add a new gropnode to the context. Caller fills in
 * all the grop's parameters (ctx->current->foo) afterwards.
 * addgropsz() fills in size parameters for ya
 */
g_error addgrop(struct gropctxt *ctx, u16 type);
g_error addgropsz(struct gropctxt *ctx, u16 type, s16 x,s16 y,s16 w,s16 h);

/***************** grop functions */

void grop_addnode(struct gropnode **headpp,struct gropnode *node);
void grop_free(struct gropnode **headpp);
void grop_kill_zombies(void);
g_error gropnode_alloc(struct gropnode **n);
void gropnode_free(struct gropnode *n);

void align(struct gropctxt *d,alignt align,s16 *w,s16 *h,s16 *x,s16 *y);

/* This renders a divnode's groplist to the screen
 *
 * Sets up a groprender structure based on the divnode's information,
 * performs pre-render housekeeping, processes flags, and renders each node
 *
 * If 'clip' is non-NULL, override the usual clipping rectangle with this one.
 */
void grop_render(struct divnode *div, struct pgquad *clip);

/* The below functions are steps used within the rendering process. Not
 * really useful by themselves, but this should help divide the monstrosity
 * that grop_render was becoming
 */

void groplist_scroll(struct groprender *r, struct divnode *div);
void gropnode_nonvisual(struct groprender *r, struct gropnode *n);
void gropnode_map(struct groprender *r, struct gropnode *n);
void gropnode_translate(struct groprender *r, struct gropnode *n);
void gropnode_clip(struct groprender *r, struct gropnode *n);
void gropnode_draw(struct groprender *r, struct gropnode *n);

/***************** geometry functions */

void quad_intersect(struct pgquad *dest, struct pgquad *src);
struct pgquad *rect_to_quad(struct pgrect *rect);
struct pgpair *xy_to_pair(s16 x, s16 y);

#endif /* __RENDER_H */

/* The End */
