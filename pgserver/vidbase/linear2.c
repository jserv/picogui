/* $Id: linear2.c,v 1.5 2001/04/29 17:28:39 micahjd Exp $
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
 *   Pascal Bauermeister <pascal.bauermeister@smartdata.ch>
 *   2001-03-23 180 degree rotation
 * 
 */

#include <pgserver/common.h>
#include <pgserver/inlstring.h>    /* inline-assembly __memcpy if possible*/
#include <pgserver/video.h>

/* Macros to easily access the members of vid->display */
#define FB_MEM     (((struct stdbitmap*)dest)->bits)
#define FB_BPL     (((struct stdbitmap*)dest)->pitch)

/* Macro for addressing framebuffer pixels. Note that this is only
 * used when an accumulator won't do, but it is a macro so a line address
 * lookup table might be implemented later if really needed.
 */
#define LINE(y)        ((y)*FB_BPL+FB_MEM)
#define PIXELBYTE(x,y) (((x)>>2)+LINE(y))

/* Table of masks used to isolate one pixel within a byte */
unsigned const char notmask2[] = { 0x3F, 0xCF, 0xF3, 0xFC };

/************************************************** Minimum functionality */

void linear2_pixel(hwrbitmap dest, s16 x,s16 y,hwrcolor c,s16 lgop) {
   char *p;
   if (lgop != PG_LGOP_NONE) {
      def_pixel(dest,x,y,c,lgop);
      return;
   }
   p = PIXELBYTE(x,y);
   *p &= notmask2[x&3];
   *p |= c << ((3-(x&3))<<1);
}
hwrcolor linear2_getpixel(hwrbitmap dest, s16 x, s16 y) {
   return ((*PIXELBYTE(x,y)) >> ((3-(x&3))<<1)) & 0x03;
}
   
/*********************************************** Accelerated (?) primitives */


/*********************************************** Registration */

/* Load our driver functions into a vidlib */
void setvbl_linear2(struct vidlib *vid) {
  setvbl_default(vid);

  vid->pixel          = &linear2_pixel;
  vid->getpixel       = &linear2_getpixel;
}

/* The End */

