/* $Id: linear1.c,v 1.1 2001/02/23 04:44:47 micahjd Exp $
 *
 * Video Base Library:
 * linear1.c - For 1-bit packed pixel devices (most black and white displays)
 *
 * BIG FAT WARNING:
 * This is just a stub that only implements pixel() getpixel() and the color
 * conversions. A fast linear1 is on the way, but this will substitute for now.
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
#define PIXELBYTE(x,y) (((x)>>3)+LINE(y))

/* Table of masks used to isolate one pixel within a byte */
unsigned const char notmask1[] = { 0x7F, 0xBF, 0xDF, 0xEF,
                                  0xF7, 0xFB, 0xFD, 0xFE };
unsigned const char pxlmask1[] = { 0x80, 0x40, 0x20, 0x10,
                                  0x08, 0x04, 0x02, 0x01 };

/************************************************** Minimum functionality */

/* Just black or white, 0 or 1... */
hwrcolor linear1_color_pgtohwr(pgcolor c) {
   return (getred(c)+getgreen(c)+getblue(c)) >= 382;
}
pgcolor linear1_color_hwrtopg(hwrcolor c) {
   return c ? 0xFFFFFF : 0x000000;
}

/* Ugh. Evil but necessary... I suppose... */

void linear1_pixel(int x,int y,hwrcolor c) {
   char *p = PIXELBYTE(x,y);
   if (c)
     *p |= pxlmask1[x&3];
   else
     *p &= notmask1[x&3];
}
  hwrcolor linear1_getpixel(int x,int y) {
   return ((*PIXELBYTE(x,y)) >> (7-(x&3))) & 1;
}
   
/*********************************************** Accelerated (?) primitives */


   
/*********************************************** Registration */

/* Load our driver functions into a vidlib */
void setvbl_linear1(struct vidlib *vid) {
  /* Start with the defaults */
  setvbl_default(vid);

  /* Minimum functionality */
  vid->pixel          = &linear1_pixel;
  vid->getpixel       = &linear1_getpixel;
  vid->color_pgtohwr  = &linear1_color_pgtohwr;
  vid->color_hwrtopg  = &linear1_color_hwrtopg;
   
  /* Accelerated functions */
//  vid->slab           = &linear1_slab;
//  vid->bar            = &linear1_bar;
//  vid->line           = &linear1_line;
//  vid->rect           = &linear1_rect;
//  vid->gradient       = &linear1_gradient;
//  vid->dim            = &linear1_dim;
//  vid->scrollblit     = &linear1_scrollblit;
//  vid->charblit       = &linear1_charblit;
//  vid->charblit_v     = &linear1_charblit_v;
//  vid->tileblit       = &linear1_tileblit;
//  vid->blit           = &linear1_blit;
//  vid->unblit         = &linear1_unblit;
}

/* The End */

