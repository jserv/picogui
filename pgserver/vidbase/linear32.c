/* $Id: linear32.c,v 1.1 2001/04/29 18:00:21 micahjd Exp $
 *
 * Video Base Library:
 * linear32.c - For 32bpp linear framebuffers
 *
 * BIG FAT WARNING:
 * This is just a stub that only implements pixel() getpixel() and the color
 * conversions. A fast linear32 is on the way, but this will substitute for now.
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

/* Macros to easily access the members of vid->display */
#define FB_MEM     (((struct stdbitmap*)dest)->bits)
#define FB_BPL     (((struct stdbitmap*)dest)->pitch)

/* Macro for addressing framebuffer pixels. Note that this is only
 * used when an accumulator won't do, but it is a macro so a line address
 * lookup table might be implemented later if really needed.
 */
#define LINE(y)        ((u32 *)((y)*FB_BPL+FB_MEM))
#define PIXELADDR(x,y) ((x)+LINE(y))
#define PIXEL(x,y)     (*PIXELADDR(x,y))

/************************************************** Minimum functionality */

void linear32_pixel(hwrbitmap dest, s16 x,s16 y,hwrcolor c,s16 lgop) {
   if (lgop != PG_LGOP_NONE) {
      def_pixel(dest,x,y,c,lgop);
      return;
   }
   PIXEL(x,y) = c;
}
hwrcolor linear32_getpixel(hwrbitmap dest, s16 x,s16 y) {
  return PIXEL(x,y);
}

/*********************************************** Accelerated (?) primitives */


   
/*********************************************** Registration */

/* Load our driver functions into a vidlib */
void setvbl_linear32(struct vidlib *vid) {
  setvbl_default(vid);
   
  vid->pixel          = &linear32_pixel;
  vid->getpixel       = &linear32_getpixel;
}

/* The End */

