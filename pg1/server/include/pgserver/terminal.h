/* $Id$
 *
 * terminal.h - Header file shared by components of the terminal emulator widget
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

#ifndef __TERMINAL_H
#define __TERMINAL_H

#include <pgserver/handle.h>
#include <pgserver/widget.h>
#include <pgserver/render.h>

/******************************************************** Data structures **/

/* Size of buffer for VT102 escape codes.
 * Normally this would only need to be 32 or so, but we need the extra
 * length for properly accepting xterm extended escapes.
 */
#define ESCAPEBUF_SIZE 256

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
  char charset;                  /* Currently selected character set */
  unsigned int reverse_video:1;
  unsigned int cursor_hidden:1;
  unsigned int no_autowrap:1;
  unsigned int key_prefix_switch:1;
  unsigned int insert_mode:1;
  unsigned int x10_mouse:1;
  unsigned int x11_mouse:1;
  unsigned int crlf_mode:1;
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
  handle htextcolors;                  /* If nonzero, a custom palette to override the theme */

  u8 escapebuf[ESCAPEBUF_SIZE];        /* Escape code buffer */
  int escbuf_pos;                      /* Position in buffer */

  int csiargs[CSIARGS_SIZE];           /* Parameter buffer for processed CSI codes */
  int num_csiargs;

  struct gropnode *bg,*bginc,*bgsrc;   /* Background */

  struct gropnode *inc;                /* The incremental gropnode */
  struct gropnode *grid;               /* Nonincremental textgrid */
  int x,y;                             /* Base coordinates */
  s16 celw,celh;                       /* Character cel size */
  int fontmargin;
  int updx,updy,updw,updh;             /* Update rectangle (in characters) */
  int pref_lines;                      /* Preferred height in lines */
  unsigned int on : 1;                 /* Mouse button down? */
  unsigned int clamp_flag : 1;         /* Nonzero to clamp at screen edges, zero to wrap/scroll */
  
  unsigned int cursor_on : 1;          /* Cursor visible? */
  unsigned int focus : 1;              /* Do we have keyboard focus? */
  unsigned int escapemode : 1;         /* Handling an escape code? */
  unsigned int autoscroll : 1;         /* Automatically scroll on cursor movement */
 
  u8 attr_default, attr_cursor;        /* Theme settings */
  u32 flashtime_on,flashtime_off;
  u32 cursor_wait;

  int mouse_x, mouse_y;                /* Current mouse position in the widget */
  int key_mods;                        /* Current key modifiers */

  struct terminal_state current;       /* Current emulated terminal state */
  struct terminal_state saved;         /* Saved terminal state, via ESC 7 and ESC 8 */
};


/******************************************************** Textgrid utilities **/

/* Render a textgrid gropnode, called from the rendering
 * engine to handle PG_GROP_TEXTGRID 
 */
void textgrid_render(struct groprender *r, struct gropnode *n);

/* Handle drawing line-drawing characters that were mapped into
 * the character set where ISO Latin-1 normally is :)
 * Return 1 if the character is handled by this function.
 */
int term_linedraw(hwrbitmap dest, int x, int y, int w, int h, 
		  unsigned char ch, hwrcolor color, int lgop);

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

/* Shift all the text at and after the cursor right by 'n' characters. */
void term_insert(struct widget *self, int n);

/* Shift all the text after the cursor left by 'n' characters. */
void term_delete(struct widget *self, int n);

/* Set a palette entry, making a mutable copy of the palette if necessary */
void term_setpalette(struct widget *self, int n, pgcolor color);


/******************************************************** VT102 emulation **/

/* Handle a single key event as received from the server */
void kbd_event(struct widget *self, int pgkey,int mods);

/* Output formatted char */
void term_char(struct widget *self,u8 c);

/* Reset the terminal emulator */
void term_reset(struct widget *self);

/* Send an x10/x11 mouse reporting code.
 *   - 'press' is a boolean indicating whether this is a press or release.
 *   - 'button' is a button number (0=left, 1=middle, etc.)
 *   - This expects an x,y position and modifiers saved already. Modifiers are in PGMOD_* format.
 */
void term_mouse_event(struct widget *self, int press, int button);

#endif /* __TERMINAL_H */

/* The End */
