/* $Id: dvbl_bitmap.c,v 1.3 2002/05/22 09:26:33 micahjd Exp $
 *
 * dvbl_bitmap.c - This file is part of the Default Video Base Library,
 *                 providing the basic video functionality in picogui but
 *                 easily overridden by drivers.
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
 */

#include <pgserver/common.h>

#ifdef DEBUG_VIDEO
#define DEBUG_FILE
#endif
#include <pgserver/debug.h>

#include <pgserver/video.h>
#include <pgserver/font.h>
#include <pgserver/render.h>
#include <pgserver/appmgr.h>   /* for res[PGRES_DEFAULT_FONT] */

/******* Table of available bitmap formats */

struct bitformat bitmap_formats[] = {

#ifdef CONFIG_FORMAT_PNM
     { {'P','N','M',0}, &pnm_detect, &pnm_load, NULL },
#endif

#ifdef CONFIG_FORMAT_JPEG
     { {'J','P','E','G'}, &jpeg_detect, &jpeg_load, NULL },
#endif

#ifdef CONFIG_FORMAT_BMP
     { {'B','M','P',0}, &bmp_detect, &bmp_load, NULL },
#endif

#ifdef CONFIG_FORMAT_PNG
     { {'P','N','G',0}, &png_detect, &png_load, NULL },
#endif

#ifdef CONFIG_FORMAT_GIF
     { {'G','I','F',0}, &gif_detect, &gif_load, NULL },
#endif

     { {0,0,0,0}, NULL, NULL, NULL }
};

#ifdef CONFIG_FORMAT_XBM
g_error def_bitmap_loadxbm(hwrbitmap *bmp,const u8 *data, s16 w, s16 h,
			   hwrcolor fg, hwrcolor bg) {
   s16 i,bit,x,y;
   unsigned char c;
   g_error e;
   
   e = (*vid->bitmap_new)(bmp,w,h,vid->bpp);
   errorcheck;
   
   /* Shift in the pixels! */
   for (y=0;h;h--,y++)
     for (x=0,bit=0,i=w;i>0;i--,x++) {
	if (!bit) c = *(data++);
	(*vid->pixel)(*bmp,x,y,(c&1) ? fg : bg,PG_LGOP_NONE);
	c >>= 1;
	bit++;
	bit &= 7;
     }
   return success;
}
#endif /* CONFIG_FORMAT_XBM */

/* Try loading a bitmap using each available format */
g_error def_bitmap_load(hwrbitmap *bmp,const u8 *data,u32 datalen) {
   struct bitformat *fmt = bitmap_formats;
   
   while (fmt->name[0]) {   /* Dummy record has empty name */
      if (fmt->detect && fmt->load && (*fmt->detect)(data,datalen))
	return (*fmt->load)(bmp,data,datalen);
      fmt++;
   }
   return mkerror(PG_ERRT_BADPARAM,8); /* Format not recognized by any loaders */
}

/* 90 degree anticlockwise rotation */
g_error def_bitmap_rotate90(hwrbitmap *b) {
   struct stdbitmap *destbit,*srcbit = (struct stdbitmap *) (*b);
   u8 *src,*srcline,*dest;
   int oshift,shift,mask;
   int shiftset  = 8-srcbit->bpp;
   int subpixel  = ((8/srcbit->bpp)-1);
   int subpixel2 = ((1<<srcbit->bpp)-1);
   g_error e;
   int h,i,x,y;
   hwrcolor c;
   
   /* New bitmap with width/height reversed */
   e = (*vid->bitmap_new)(&destbit,srcbit->h,srcbit->w,srcbit->bpp);
   errorcheck;
   
   src = srcline = srcbit->bits;
   for (h=srcbit->h,x=y=0;h;h--,y++,src=srcline+=srcbit->pitch) {

      /* Per-line mask calculations for <8bpp destination blits */
      if (srcbit->bpp<8) {
	 shift = (subpixel-(y&subpixel)) * srcbit->bpp;
	 mask  = subpixel2<<shift;
      }
      
      for (oshift=shiftset,i=srcbit->w,x=0;i;i--,x++) {
	 
	 /* Read in a pixel */
	 switch (srcbit->bpp) {
	  case 1:
	  case 2:
	  case 4:
	    c = ((*src) >> oshift) & subpixel2;
	    if (!oshift) {
	       oshift = shiftset;
	       src++;
	    }
	    else
	      oshift -= srcbit->bpp;
	    break; 
	    
	  case 8:
	    c = *(src++);
	    break;
	    
	  case 16:
	    c = *(((u16*)src)++);
	    break;
	    
	  case 24:
	    c = src[2] | (src[1]<<8) | (src[0]<<16);
	    src += 3;
	    break;
	    
	  case 32:
	    c = *(((u32*)src)++);
	    break;     
	 }
	 
	 /* Plot the pixel */
	 dest = destbit->bits + ((y*srcbit->bpp)>>3) + 
	   (srcbit->w-1-x)*destbit->pitch;
	 switch (srcbit->bpp) {
	  case 1:
	  case 2:
	  case 4:
	    *dest &= ~mask;
	    *dest |= (c << shift) & mask;
	    break; 
	    
	  case 8:
	    *dest = c;
	    break;
	    
	  case 16:
	    *((u16*)dest) = c;
	    break;
	    
	  case 24:
	    *(dest++) = (u8) c;
	    *(dest++) = (u8) (c >> 8);
	    *dest     = (u8) (c >> 16);
	    break;
	    
	  case 32:
	    *((u32*)dest) = c;
	    break;     
	 }	
      }   
   }
   
   /* Clean up */
   *b = (hwrbitmap) destbit;
   (*vid->bitmap_free)(srcbit);
   return success;
}

g_error def_bitmap_new(hwrbitmap *b, s16 w,s16 h,u16 bpp) {
  g_error e;
  struct stdbitmap **bmp = (struct stdbitmap **) b;
  int lw;
  u32 size;
   
  /* The bitmap and the header can be allocated seperately,
     but usually it is sufficient to make them one big
     chunk of memory.  It's 1 alloc vs 2.
  */
  
  /* Pad the line width up to the nearest byte */
  lw = (u32) w * bpp;
  if ((bpp<8) && (lw & 7))
     lw += 8;
  lw >>= 3;

  /* The +1 is to make blits for < 8bpp simpler. Shouldn't really
   * be necessary, though. FIXME */
  size = sizeof(struct stdbitmap) + (lw * h) + 1;

  e = g_malloc((void **) bmp,size);
  errorcheck;
  memset(*bmp,0,size);
  (*bmp)->pitch = lw;
  (*bmp)->bits = ((unsigned char *)(*bmp)) + 
    sizeof(struct stdbitmap);
  (*bmp)->w = w;
  (*bmp)->h = h;
  (*bmp)->bpp = bpp;
  
  return success;
}

void def_bitmap_free(struct stdbitmap *bmp) {
  if (!bmp) return;
  if (bmp->rend)
    g_free(bmp->rend);
  if (bmp->freebits)
    g_free(bmp->bits);
  g_free(bmp);
}

g_error def_bitmap_getsize(hwrbitmap b,s16 *w,s16 *h) {
   struct stdbitmap *bmp = (struct stdbitmap *) b;
   *w = bmp->w;
   *h = bmp->h;
   return success;
}


/* Optional
 *   This is called for every bitmap when entering a new bpp or loading
 *   a new driver. Converts a bitmap from a linear array of 32-bit
 *   pgcolor values to the hwrcolors for this mode
 *
 * Default implementation: stdbitmap
 */
g_error def_bitmap_modeconvert(struct stdbitmap **bmp) {
   struct stdbitmap *destbit,*srcbit = *bmp;
   u32 *src;
   u8 *destline,*dest;
   int oshift;
   int shiftset  = 8-vid->bpp;
   g_error e;
   int h,i,x,y;
   hwrcolor c;

   /* FIXME: this should check whether the image has an alpha channel */
   
   /* New bitmap at our present bpp */
   e = (*vid->bitmap_new)(&destbit,srcbit->w,srcbit->h,vid->bpp);
   errorcheck;

   src = (u32 *) srcbit->bits;
   dest = destline = destbit->bits;
   
   for (h=srcbit->h,x=y=0;h;h--,y++,dest=destline+=destbit->pitch) {      
      for (oshift=shiftset,i=srcbit->w,x=0;i;i--,x++) {	 
	 /* Read in a pixel */
	 c = (*vid->color_pgtohwr)(*(src++));
	 
	 /* Output in new device bpp */
	 switch (vid->bpp) {
	  case 1:
	  case 2:
	  case 4:
	    if (oshift==shiftset)
	      *dest = c << oshift;
	    else
	      *dest |= c << oshift;
	    if (!oshift) {
	       oshift = shiftset;
	       dest++;
	    }
	    else
	      oshift -= vid->bpp;
	    break;
	    
	  case 8:
	    *(((unsigned char *)dest)++) = c;
	    break;
	    
	  case 16:
	    *(((unsigned short *)dest)++) = c;
	    break;
	    
	  case 24:
	    *(dest++) = (unsigned char) c;
	    *(dest++) = (unsigned char) (c >> 8);
	    *(dest++) = (unsigned char) (c >> 16);
	    break;
	    
	  case 32:
	    *(((u32 *)dest)++) = c;
	    break;
	 }
      }   
   }
   
   /* Clean up */
   *bmp = destbit;
   (*vid->bitmap_free)(srcbit);
   return success;
}
   
/* Optional
 *   The reverse of bitmap_modeconvert, this converts the bitmap from
 *    the hardware-specific format to a pgcolor array
 * 
 * Default implementation: stdbitmap
 */
g_error def_bitmap_modeunconvert(struct stdbitmap **bmp) {
   struct stdbitmap *destbit,*srcbit;
   u8 *src,*srcline;
   u32 *dest;  /* Must be word-aligned, this is always the case if
		* struct stdbitmap is padded correctly */
   int oshift;
   int shiftset  = 8-vid->bpp;
   g_error e;
   int h,i,x,y;
   hwrcolor c;

   if ((*bmp)->bpp == 32)
     return success;
   
   /* New bitmap at 32bpp (this is hackish, but I don't see anything
    * seriously wrong with it... yet...) 
    */
   srcbit = *bmp;
   e = (*vid->bitmap_new)(&destbit,srcbit->w,srcbit->h,32);
   errorcheck;
   
   src = srcline = srcbit->bits;
   dest = (u32 *) destbit->bits;
   
   for (h=srcbit->h,x=y=0;h;h--,y++,src=srcline+=srcbit->pitch) {      
      for (oshift=shiftset,i=srcbit->w,x=0;i;i--,x++) {
	 
	 /* Read in a pixel */
	 switch (vid->bpp) {
	  case 1:
	  case 2:
	  case 4:
	    c = ((*src) >> oshift) & ((1<<vid->bpp)-1);
	    if (!oshift) {
	       oshift = shiftset;
	       src++;
	    }
	    else
	      oshift -= vid->bpp;
	    break; 
	    
	  case 8:
	    c = *(src++);
	    break;
	    
	  case 16:
	    c = *(((u16*)src)++);
	    break;
	    
	  case 24:
	    c = (*(src++)) | ((*(src++))<<8) | ((*(src++))<<16);
	    break;
	    
	  case 32:
	    c = *(((u32*)src)++);
	    break;     
	 }
	 
	 *(dest++) = (*vid->color_hwrtopg)(c);
      }   
   }
   
   /* Clean up */
   *bmp = destbit;
   (*vid->bitmap_free)(srcbit);
   return success;
}

g_error def_bitmap_get_groprender(hwrbitmap bmp, struct groprender **rend) {
  struct stdbitmap *b = (struct stdbitmap *) bmp;
  g_error e;
  s16 w,h;

  if (b->rend) {
    *rend = b->rend;
    return success;
  }

  /* ack... we need to make a new context */

  def_bitmap_getsize(bmp,&w,&h);

  e = g_malloc((void **) rend,sizeof(struct groprender));
  errorcheck;
  b->rend = *rend;
  memset(*rend,0,sizeof(struct groprender));
  (*rend)->lgop = PG_LGOP_NONE;
  (*rend)->output = bmp;
  (*rend)->hfont = res[PGRES_DEFAULT_FONT];
  (*rend)->clip.x2 = w - 1;
  (*rend)->clip.y2 = h - 1;
  (*rend)->orig_clip = (*rend)->clip;
  (*rend)->output_rect.w = w;
  (*rend)->output_rect.h = h;

  return success;
}

#ifndef min
#define min(a,b) (((a)<(b))?(a):(b))
#endif

void def_tileblit(hwrbitmap dest, s16 x, s16 y, s16 w, s16 h,
		  hwrbitmap src, s16 sx, s16 sy, s16 sw, s16 sh, s16 lgop) {
  s16 i,j;

  if (!(sw && sh)) return;
   
  /* Do a tiled blit */
  for (i=0;i<w;i+=sw)
     for (j=0;j<h;j+=sh)
       (*vid->blit) (dest,x+i,y+j,min(w-i,sw),min(h-j,sh),
		     src,sx,sy,lgop);
}

/* Scary slow blit, but necessary for dealing with unsupported lgop values
 * or other things that the fast lib can't deal with */
void def_blit(hwrbitmap dest, s16 x,s16 y,s16 w,s16 h, hwrbitmap src,
	      s16 src_x, s16 src_y, s16 lgop) {
   int i;
   s16 bw,bh;
   
   if(!src)
	   return;

   (*vid->bitmap_getsize)(src,&bw,&bh);

   if (w>(bw-src_x) || h>(bh-src_y)) {
      int i,j,sx,sy;
      src_x %= bw;
      src_y %= bh;
      
      /* Do a tiled blit */
      for (i=0,sx=src_x;i<w;i+=bw-sx,sx=0)
	for (j=0,sy=src_y;j<h;j+=bh-sy,sy=0)
	  (*vid->blit) (dest,x+i,y+j,
			min(bw-sx,w-i),min(bh-sy,h-j),
			src,sx,sy,lgop);
      return;
   }
   
   /* Icky blit loop */
   for (;h;h--,y++,src_y++)
     for (i=0;i<w;i++)
	(*vid->pixel) (dest,x+i,y,(*vid->getpixel)(src,src_x+i,src_y),lgop);
}

/* Backwards version of the scary slow blit, needed for scrolling 1/2 of the time */
void def_scrollblit(hwrbitmap dest, s16 x,s16 y,s16 w,s16 h, hwrbitmap src,
		    s16 src_x, s16 src_y, s16 lgop) {
  /* Special scrollblit handling is only necessary if we're copying to the same bitmap */
  if (dest==src) {

    /* If the blit moves the image down, we need to to an upside-down blit */
    if (y>src_y) {
      for (y+=h-1,src_y+=h-1;h;h--,y--,src_y--)
	(*vid->blit) (dest,x,y,w,1,src,src_x,src_y,lgop);
      return;
    }
    
    /* If the blit moves right on the same line, we can split the image into vertical
     * slices to do the horizontal equivalent of an upside-down blit. Note that though
     * this seems obscure, it's the common method of scrolling on rotated platforms such
     * as most QVGA handheld devices.
     */
    if (y==src_y && x>src_x) {
      for (x+=w-1,src_x+=w-1;w;w--,x--,src_x--)
	(*vid->blit) (dest,x,y,1,h,src,src_x,src_y,lgop);
      return;
    }
  }    
  
  /* Well... no reason we can't do a normal blit */
  (*vid->blit) (dest,x,y,w,h,src,src_x,src_y,lgop);
}
   
/* The End */
