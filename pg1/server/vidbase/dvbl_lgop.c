/* $Id$
 *
 * dvbl_lgop.c - This file is part of the Default Video Base Library,
 *               providing the basic video functionality in picogui but
 *               easily overridden by drivers.
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2003 Micah Dowty <micahjd@users.sourceforge.net>
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
 */

#include <pgserver/common.h>

#ifdef DEBUG_VIDEO
#define DEBUG_FILE
#endif
#include <pgserver/debug.h>

#include <pgserver/video.h>
#include <pgserver/font.h>
#include <pgserver/render.h>

void def_pixel(hwrbitmap dest, s16 x, s16 y, hwrcolor c, s16 lgop){
   switch (lgop) {
      
    case PG_LGOP_NONE:   /* Generic access to packed-pixel bitmaps */
	{
	   struct stdbitmap *bmp = (struct stdbitmap *) dest;
	   u8 *dst = bmp->bits + bmp->pitch*y + ((x*bmp->bpp)>>3);
	   u8 shift,mask;
	   s16 subpixel,subpixel2;
	   switch (bmp->bpp) {
	    case 1:
	    case 2:
	    case 4:
	      subpixel  = ((8/bmp->bpp)-1);
	      subpixel2 = ((1<<bmp->bpp)-1);
	      shift = (subpixel-(x&subpixel)) * bmp->bpp;
	      mask  = subpixel2<<shift;
	      *dst &= ~mask;
	      *dst |= (c << shift) & mask;
	      break; 
	      
	    case 8:
	      *dst = c;
	      break;
	      
	    case 16:
	      *((u16*)dst) = c;
	      break;
	      
	    case 24:
	      *(dst++) = (u8) c;
	      *(dst++) = (u8) (c >> 8);
	      *dst     = (u8) (c >> 16);
	      break;
	      
	    case 32:
	      *((u32*)dst) = c;
	      break;     
	   }
	}
      break;

      /* LGOP implementations */

    case PG_LGOP_OR:
      (*vid->pixel)(dest,x,y,  
		    (*vid->getpixel)(dest,x,y) | c,
		    PG_LGOP_NONE);
      break;
      
    case PG_LGOP_AND:
      (*vid->pixel)(dest,x,y,  
		    (*vid->getpixel)(dest,x,y) & c,
		    PG_LGOP_NONE);
      break;
      
    case PG_LGOP_XOR:
      (*vid->pixel)(dest,x,y,  
		    (*vid->getpixel)(dest,x,y) ^ c,
		    PG_LGOP_NONE);
      break;
      
    case PG_LGOP_INVERT:
      (*vid->pixel)(dest,x,y,  
		    (c ^ (~((u32)0))),
		    PG_LGOP_NONE);
      break;
      
    case PG_LGOP_INVERT_OR:
      (*vid->pixel)(dest,x,y,  
		    (*vid->getpixel)(dest,x,y) | (c ^ (~((u32)0))),
		    PG_LGOP_NONE);
      break;
      
    case PG_LGOP_INVERT_AND:
      (*vid->pixel)(dest,x,y,  
		    (*vid->getpixel)(dest,x,y) & (c ^ (~((u32)0))),
		    PG_LGOP_NONE);
      break;
      
    case PG_LGOP_INVERT_XOR:
      (*vid->pixel)(dest,x,y,  
		    (*vid->getpixel)(dest,x,y) ^ (c ^ (~((u32)0))),
		    PG_LGOP_NONE);
      break;
      
    case PG_LGOP_ADD:
	{
	   s16 r,g,b;
	   pgcolor oc = (*vid->color_hwrtopg) ((*vid->getpixel)(dest,x,y));
	   c = (*vid->color_hwrtopg) (c);
	   
	   r = getred(oc);
	   g = getgreen(oc);
	   b = getblue(oc);
	   r += getred(c);
	   g += getgreen(c);
	   b += getblue(c);
	   
	   if (r>255) r=255;
	   if (g>255) g=255;
	   if (b>255) b=255;
	   
	   (*vid->pixel) (dest,x,y,(*vid->color_pgtohwr)(mkcolor(r,g,b)),
			  PG_LGOP_NONE);
	}
      break;
      
    case PG_LGOP_SUBTRACT:
	{
	   s16 r,g,b;
	   pgcolor oc = (*vid->color_hwrtopg) ((*vid->getpixel)(dest,x,y));
	   c = (*vid->color_hwrtopg) (c);
	   
	   r = getred(oc);
	   g = getgreen(oc);
	   b = getblue(oc);
	   r -= getred(c);
	   g -= getgreen(c);
	   b -= getblue(c);
	   
	   if (r<0) r=0;
	   if (g<0) g=0;
	   if (b<0) b=0;
	   
	   (*vid->pixel) (dest,x,y,(*vid->color_pgtohwr)(mkcolor(r,g,b)),
			  PG_LGOP_NONE);
	}
      break;
      
    case PG_LGOP_MULTIPLY:
	{
	   s16 r,g,b;
	   pgcolor oc = (*vid->color_hwrtopg) ((*vid->getpixel)(dest,x,y));
	   c = (*vid->color_hwrtopg) (c);
	   
	   r = getred(oc) * getred(c) / 255;
	   g = getgreen(oc) * getgreen(c) / 255;
	   b = getblue(oc) * getblue(c) / 255;
	   
	   (*vid->pixel) (dest,x,y,(*vid->color_pgtohwr)(mkcolor(r,g,b)),
			  PG_LGOP_NONE);
	}
      break;

    case PG_LGOP_STIPPLE:
      if ((x+y)&1)
	(*vid->pixel) (dest,x,y,c,PG_LGOP_NONE);
      break;

   case PG_LGOP_ALPHA:
     {
       /* Alpha blending, assuming c is a hwrcolor with the PGCF_ALPHA flag.
	* The color has already been premultiplied.
	*/
       pgcolor oc = (*vid->color_hwrtopg) ((*vid->getpixel)(dest,x,y));
       u8 r,g,b,a;

       a = 128 - getalpha(c);
       r = getred(c) + ((getred(oc) * a) >> 7);
       g = getgreen(c) + ((getgreen(oc) * a) >> 7);
       b = getblue(c) + ((getblue(oc) * a) >> 7);

       (*vid->pixel) (dest,x,y,(*vid->color_pgtohwr)(mkcolor(r,g,b)),
		      PG_LGOP_NONE); 
     }
   }
}
   
hwrcolor def_getpixel(hwrbitmap src, s16 x, s16 y) {
   struct stdbitmap *bmp = (struct stdbitmap *) src;
   u8 *s = bmp->bits + bmp->pitch*y + ((x*bmp->bpp)>>3);
   u8 subpixel,shift;
   switch (bmp->bpp) {
    case 1:
    case 2:
    case 4:
      subpixel  = ((8/bmp->bpp)-1);
      shift = (subpixel-(x&subpixel)) * bmp->bpp;
      return ((*s) >> shift) & ((1<<bmp->bpp)-1);
      
    case 16:
      return *((u16*)s);
      
    case 24:
      return s[2] | (s[1]<<8) | (s[0]<<16);
      
    case 32:
      return *((u32*)s);
   }
   /* 8 */
   return *s;
}

void def_slab(hwrbitmap dest, s16 x,s16 y,s16 w,hwrcolor c,s16 lgop) {
  /* You could make this create a very thin rectangle, but then if niether
   * were implemented they would be a pair of mutually recursive functions! */
   
  for (;w;w--,x++)
     (*vid->pixel) (dest,x,y,c,lgop);
}

void def_bar(hwrbitmap dest,s16 x,s16 y,s16 h,hwrcolor c,s16 lgop) {
  (*vid->rect) (dest,x,y,1,h,c,lgop);
}

void def_rect(hwrbitmap dest,s16 x,s16 y,s16 w,s16 h,hwrcolor c,s16 lgop) {
  for (;h;h--,y++) (*vid->slab) (dest,x,y,w,c,lgop);
}

/* The End */
