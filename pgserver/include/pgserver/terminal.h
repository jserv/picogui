/* $Id: terminal.h,v 1.2 2002/10/11 15:40:17 micahjd Exp $
 *
 * terminal.h - Header file shared by components of the terminal emulator widget
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2002 Micah Dowty <micahjd@users.sourceforge.net>
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

#ifndef __TERMINAL_H
#define __TERMINAL_H

#include <pgserver/handle.h>
#include <pgserver/widget.h>
#include <pgserver/render.h>

/******************************************************** Data structures **/

/* Size of buffer for VT102 escape codes */
#define ESCAPEBUF_SIZE 32

/* Maximum # of params for a CSI escape code.
 * Same limit imposed by the linux console, should be fine */
#define CSIARGS_SIZE   16

/* This is the state of the emulated terminal 
 */
struct terminal_state {
  int crsrx,crsry;               /* Current cursor location */
  int savcrsrx, savcrsry;        /* Cursor location saved with ESC [ s */
  u8 attr;                       /* Default attribute for new characters */
  int scroll_top, scroll_bottom; /* Scrolling region set with CSI r */
  char g[4];                     /* Character set selections */
  unsigned int cursor_hidden:1;
  unsigned int no_autowrap:1;
};

/* All internal data for the terminal widget, accessed with the DATA macro 
 */
struct terminaldata {
  u32 update_time;                     /* Time of the last update, used with CURSOR_WAIT */
  handle font,deffont;                 /* Font currently set, and default fixed-width font */    
  u8 attr_under_crsr;                  /* Saved attribute under the cursor */

  handle hbuffer;                      /* Text buffer */
  struct pgstring *buffer;
  int bufferw,bufferh;

  u8 escapebuf[ESCAPEBUF_SIZE];        /* Escape code buffer */
  int escbuf_pos;                      /* Position in buffer */

  int csiargs[CSIARGS_SIZE];           /* Parameter buffer for processed CSI codes */
  int num_csiargs;

  struct gropnode *bg,*bginc,*bgsrc;   /* Background */

  struct gropnode *inc;                /* The incremental gropnode */
  int x,y;                             /* Base coordinates */
  s16 celw,celh;                       /* Character cel size */
  int fontmargin;
  int updx,updy,updw,updh;             /* Update rectangle (in characters) */
  int pref_lines;                      /* Preferred height in lines */
  unsigned int on : 1;                 /* Mouse button down? */
  
  unsigned int cursor_on : 1;          /* Cursor visible? */
  unsigned int focus : 1;              /* Do we have keyboard focus? */
  unsigned int escapemode : 1;         /* Handling an escape code? */
  unsigned int autoscroll : 1;         /* Automatically scroll on cursor movement */
 
  u8 attr_default, attr_cursor;        /* Theme settings */
  u32 flashtime_on,flashtime_off;
  u32 cursor_wait;

  struct terminal_state current;       /* Current emulated terminal state */
  struct terminal_state saved;         /* Saved terminal state, via ESC 7 and ESC 8 */
};


/******************************************************** Textgrid utilities **/

/* Render a textgrid gropnode, called from the rendering
 * engine to handle PG_GROP_TEXTGRID 
 */
void textgrid_render(struct groprender *r, struct gropnode *n);

/* Create an incremental gropnode for the update rectangle */
void term_realize(struct widget *self);

/* Prepare for adding more update rectangles */
void term_rectprepare(struct widget *self);

/* Add update rectangle */
void term_updrect(struct widget *self,int x,int y,int w,int h);

/* Plot a character at an x,y position, add update rect */
void term_plot(struct widget *self,int x,int y,u8 c);

/* Change attribute at an x,y position, return old attribute */
u8 term_chattr(struct widget *self,int x,int y,u8 c);

/* Hide/show cursor */
void term_setcursor(struct widget *self,int flag);

/* Clear a chunk of buffer */
void term_clearbuf(struct widget *self,int fromx,int fromy,int chars);

/* Scroll the specified region up/down by some number of lines,
 * clearing the newly exposed region. ('lines' is + for down, - for up)
 */
void term_scroll(struct widget *self, int top_y, int bottom_y, int lines);

/* Copy a rectangle between two text buffers */
void textblit(struct pgstring *src,struct pgstring *dest,
	      int src_x,int src_y,int src_w,
	      int dest_x,int dest_y,int dest_w,int w,int h);


/******************************************************** VT102 emulation **/

/* Handle a single key event as received from the server */
void kbd_event(struct widget *self, int pgkey,int mods);

/* Output formatted char */
void term_char(struct widget *self,u8 c);

#endif /* __TERMINAL_H */

/* The End */
