/* $Id$
 *
 * Video Base Library:
 * linear24.c - For 24bpp linear framebuffers
 *
 * BIG FAT WARNING:
 * This is just a stub that only implements pixel() getpixel() and the color
 * conversions. A fast linear24 is on the way, but this will substitute for now.
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

#include <pgserver/common.h>

#include <pgserver/inlstring.h>    /* inline-assembly __memcpy */
#include <pgserver/video.h>

/* Macros to easily access the destination bitmap */
#define FB_MEM     (((struct stdbitmap*)dest)->bits)
#define FB_BPL     (((struct stdbitmap*)dest)->pitch)
#define FB_ISNORMAL(bmp,lgop) (lgop == PG_LGOP_NONE && ((struct stdbitmap*)bmp)->bpp == vid->bpp)

/* Macro for addressing framebuffer pixels. Note that this is only
 * used when an accumulator won't do, but it is a macro so a line address
 * lookup table might be implemented later if really needed.
 */
#define LINE(y)        ((u8 *)((y)*FB_BPL+FB_MEM))
#define PIXELADDR(x,y) (((x)*3)+LINE(y))

/************************************************** Minimum functionality */

void linear24_pixel(hwrbitmap dest, s16 x,s16 y,hwrcolor c,s16 lgop) {
   u8 *p;
   
   if (!FB_ISNORMAL(dest,lgop)) {
      def_pixel(dest,x,y,c,lgop);
      return;
   }

   p = PIXELADDR(x,y);
   *(p++) = (u8) c;
   *(p++) = (u8) (c >> 8);
   *p     = (u8) (c >> 16);
}
hwrcolor linear24_getpixel(hwrbitmap dest, s16 x,s16 y) {
   u8 *s;

   if (!FB_ISNORMAL(dest,PG_LGOP_NONE))
     return def_getpixel(dest,x,y);  

   s = PIXELADDR(x,y);
   return s[0] | (s[1]<<8) | (s[2]<<16);
}

/*********************************************** Accelerated (?) primitives */


   
/*********************************************** Registration */

/* Load our driver functions into a vidlib */
void setvbl_linear24(struct vidlib *vid) {
  setvbl_default(vid);
   
  vid->pixel          = &linear24_pixel;
  vid->getpixel       = &linear24_getpixel;
}

/* The End */

