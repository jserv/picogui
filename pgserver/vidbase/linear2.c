/* $Id: linear2.c,v 1.2 2001/02/28 00:19:07 micahjd Exp $
 *
 * Video Base Library:
 * linear2.c - For 1-bit packed pixel devices (most black and white displays)
 *
 * BIG FAT WARNING:
 * This is just a stub that only implements pixel() getpixel() and the color
 * conversions. A fast linear2 is on the way, but this will substitute for now.
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000,2001 Micah Dowty <micahjd@users.sourceforge.net>
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

#include <pgserver/common.h>
#include <pgserver/inlstring.h>    /* inline-assembly __memcpy if possible*/
#include <pgserver/video.h>

/* Macro for addressing framebuffer pixels. Note that this is only
 * used when an accumulator won't do, but it is a macro so a line address
 * lookup table might be implemented later if really needed.
 */
#define LINE(y)        ((y)*vid->fb_bpl+vid->fb_mem)
#define PIXELBYTE(x,y) (((x)>>2)+LINE(y))

/* Table of masks used to isolate one pixel within a byte */
unsigned const char notmask2[] = { 0x3F, 0xCF, 0xF3, 0xFC };

/************************************************** Minimum functionality */

/* Assume a four-gray palette. Sorry, this won't work for CGA ;-) */
hwrcolor linear2_color_pgtohwr(pgcolor c) {
   return (getred(c)+getgreen(c)+getblue(c)) / 255;
}
pgcolor linear2_color_hwrtopg(hwrcolor c) {
   /* If this was called more often we could use a lookup table,
    * but it's not even worth the space here. */
   unsigned char gray = c * 85;
   return mkcolor(gray,gray,gray);
}

/* Ugh. Evil but necessary... I suppose... */

void linear2_pixel(int x,int y,hwrcolor c) {
   char *p = PIXELBYTE(x,y);
   *p &= notmask2[x&3];
   *p |= c << ((3-(x&3))<<1);
}
hwrcolor linear2_getpixel(int x,int y) {
   return ((*PIXELBYTE(x,y)) >> ((3-(x&3))<<1)) & 0x03;
}
   
/*********************************************** Accelerated (?) primitives */


   
/*********************************************** Registration */

/* Load our driver functions into a vidlib */
void setvbl_linear2(struct vidlib *vid) {
  /* Start with the defaults */
  setvbl_default(vid);

  /* Minimum functionality */
  vid->pixel          = &linear2_pixel;
  vid->getpixel       = &linear2_getpixel;
  vid->color_pgtohwr  = &linear2_color_pgtohwr;
  vid->color_hwrtopg  = &linear2_color_hwrtopg;
   
  /* Accelerated functions */
//  vid->slab           = &linear2_slab;
//  vid->bar            = &linear2_bar;
//  vid->line           = &linear2_line;
//  vid->rect           = &linear2_rect;
//  vid->gradient       = &linear2_gradient;
//  vid->dim            = &linear2_dim;
//  vid->scrollblit     = &linear2_scrollblit;
//  vid->charblit       = &linear2_charblit;
//  vid->charblit_v     = &linear2_charblit_v;
//  vid->tileblit       = &linear2_tileblit;
//  vid->blit           = &linear2_blit;
//  vid->unblit         = &linear2_unblit;
}

/* The End */

