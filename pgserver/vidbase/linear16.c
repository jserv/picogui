/* $Id: linear16.c,v 1.1 2001/03/01 02:23:11 micahjd Exp $
 *
 * Video Base Library:
 * linear16.c - For 16bpp linear framebuffers (5-6-5 RGB mapping)
 *
 * BIG FAT WARNING:
 * This is just a stub that only implements pixel() getpixel() and the color
 * conversions. A fast linear16 is on the way, but this will substitute for now.
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

#include <pgserver/inlstring.h>    /* inline-assembly __memcpy */
#include <pgserver/video.h>

/* Macro for addressing framebuffer pixels. Note that this is only
 * used when an accumulator won't do, but it is a macro so a line address
 * lookup table might be implemented later if really needed.
 */
#define LINE(y)        ((unsigned short *)((y)*vid->fb_bpl+vid->fb_mem))
#define PIXELADDR(x,y) ((x)+LINE(y))
#define PIXEL(x,y)     (*PIXELADDR(x,y))

/************************************************** Minimum functionality */

/* 5-6-5 RGB color quantization */
hwrcolor linear16_color_pgtohwr(pgcolor c) {
    return (((getred(c) << 8) & 0xF800) |
	    ((getgreen(c) << 3) & 0x07E0) |
	    ((getblue(c) >> 3) & 0x001F));
}
pgcolor linear16_color_hwrtopg(hwrcolor c) {
  return mkcolor( (c&0xF800) >> 8,
		  (c&0x07E0) >> 3,
		  (c&0x001F) << 3 );
}

void linear16_pixel(int x,int y,hwrcolor c) {
   PIXEL(x,y) = c;
}
hwrcolor linear16_getpixel(int x,int y) {
  return PIXEL(x,y);
}

/*********************************************** Accelerated (?) primitives */


   
/*********************************************** Registration */

/* Load our driver functions into a vidlib */
void setvbl_linear16(struct vidlib *vid) {
  /* Start with the defaults */
  setvbl_default(vid);
   
  /* Minimum functionality */
  vid->pixel          = &linear16_pixel;
  vid->getpixel       = &linear16_getpixel;
  vid->color_pgtohwr  = &linear16_color_pgtohwr;
  vid->color_hwrtopg  = &linear16_color_hwrtopg;
   
  /* Accelerated functions */
//  vid->slab           = &linear16_slab;
//  vid->bar            = &linear16_bar;
//  vid->line           = &linear16_line;
//  vid->rect           = &linear16_rect;
//  vid->gradient       = &linear16_gradient;
//  vid->dim            = &linear16_dim;
//  vid->scrollblit     = &linear16_scrollblit;
//  vid->charblit       = &linear16_charblit;
//  vid->charblit_v     = &linear16_charblit_v;
//  vid->tileblit       = &linear16_tileblit;
//  vid->blit           = &linear16_blit;
//  vid->unblit         = &linear16_unblit;
}

/* The End */

