/* $Id$
 *
 * dvbl_bitmap.c - This file is part of the Default Video Base Library,
 *                 providing the basic video functionality in picogui but
 *                 easily overridden by drivers.
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

/* This code is pretty general, if you want it to handle any angle
 * it would just need to use a fixed point rotation matrix instead of integer.
 * As it is, that's probably not needed since without proper blending it
 * would look crappy :)
 *
 * Oh, and this code is also really slow, since it uses the dreaded pixel()...
 * It is necessary for compatibility, and should serve as a good base for
 * an optimized version.
 */
void def_rotateblit(hwrbitmap dest, s16 dest_x, s16 dest_y,
		    hwrbitmap src, s16 src_x, s16 src_y, s16 src_w, s16 src_h,
		    struct pgquad *clip, s16 angle, s16 lgop) {
  int i,j,sx,sy,dx,dy;
  int a,b,c,d;   /* Rotation matrix */

  /* For each angle, set the rotation matrix */
  switch (angle) {
  case 0:
    /*   x       y        */
    a =  1; b =  0; /* x' */
    c =  0; d =  1; /* y' */
    break;

  case 90:
    /*   x       y        */
    a =  0; b =  1; /* x' */
    c = -1; d =  0; /* y' */
    break;

  case 180:
    /*   x       y        */
    a = -1; b =  0; /* x' */
    c =  0; d = -1; /* y' */
    break;

  case 270:
    /*   x       y        */
    a =  0; b = -1; /* x' */
    c =  1; d =  0; /* y' */
    break;

  default:
    return;   /* Can't handle this angle! */
  }

  /* Blitter loop, moving the source as normal,
   * but using the rotation matrix above to move the destination.
   */
  for (j=src_h,sy=src_y;j;j--,sy++) {
    dx = dest_x;
    dy = dest_y;
    for (i=src_w,sx=src_x;i;i--,sx++) {
      /* Cheesyclip (tm)
       * Hopefully the individual drivers will do this better...
       */
      if (dx >= clip->x1 && dx <= clip->x2 &&
	  dy >= clip->y1 && dy <= clip->y2)
	vid->pixel(dest,dx,dy,vid->getpixel(src,sx,sy),PG_LGOP_NONE);
      dx += a;
      dy += c;
    }
    dest_x += b;
    dest_y += d;
  }
}

g_error def_bitmap_new(hwrbitmap *b, s16 w,s16 h,u16 bpp) {
  g_error e;
  struct stdbitmap **bmp = (struct stdbitmap **) b;
  int lw;
  u32 size;
   
  /* We could allocate the bitmap and header in one chunk, but this
   * causes hairy problems with word alignment, shared memory, reallocating...
   */
  
  /* Pad the line width up to the nearest byte */
  lw = (u32) w * bpp;
  if ((bpp<8) && (lw & 7))
     lw += 8;
  lw >>= 3;

  /* The +1 is to make blits for < 8bpp simpler. Shouldn't really
   * be necessary, though. FIXME */
  size = (lw * h) + 1;

  e = g_malloc((void **) bmp,sizeof(struct stdbitmap));
  errorcheck;
  memset(*bmp,0,sizeof(struct stdbitmap));

  (*bmp)->pitch = lw;
  (*bmp)->w = w;
  (*bmp)->h = h;
  (*bmp)->bpp = bpp;
  (*bmp)->freebits = 1;
  
  e = g_malloc((void **) &(*bmp)->bits, size);
  errorcheck;

  return success;
}

void def_bitmap_free(struct stdbitmap *bmp) {
  if (!bmp) return;

  if (bmp->rend)
    g_free(bmp->rend);

  if (bmp->freebits)
    g_free(bmp->bits);

  if (bmp->shm_id)
    os_shm_free(bmp->bits, bmp->shm_id);

  g_free(bmp);
}

g_error def_bitmap_getsize(hwrbitmap b,s16 *w,s16 *h) {
   struct stdbitmap *bmp = (struct stdbitmap *) b;
   if (bmp) {
     *w = bmp->w;
     *h = bmp->h;
   }
   else {
     *w = vid->xres;
     *h = vid->yres;
   }

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

  VID(bitmap_getsize)(bmp,&w,&h);

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

/* Blit that supports tiling the source bitmap */
void def_multiblit(hwrbitmap dest, s16 x, s16 y, s16 w, s16 h,
		   hwrbitmap src, s16 sx, s16 sy, s16 sw, s16 sh, 
		   s16 xo, s16 yo, s16 lgop) {
  s16 i,j;
  int blit_x, blit_y, blit_w, blit_h, blit_src_x, blit_src_y;
  int full_line_y = -1;

  if (!(sw && sh)) return;

  /* Split the tiled blit up into individual blits clipped against the destination.
   * We do y clipping once per line, since only x coordinates change in the inner loop
   */
  
  for (j=-yo;j<h;j+=sh) {
    blit_y = y+j;
    blit_h = sh;
    blit_src_y = sy;
    if (j<0) {
      blit_y = y;
      blit_h += j;
      blit_src_y -= j;
    }
    if (blit_y + blit_h > y + h)
      blit_h = y + h - blit_y;
    
    if (lgop == PG_LGOP_NONE && full_line_y >= 0 && blit_h == sh) {
      /* If this blit isn't blended, this line is full-height, and there's already been
       * one full-height line drawn, we can copy that line instead of drawing a new one.
       * This can reduce the total number of blits from approximately (w/sw)*(h/sw) to (w/sw)+(h/sw)
       */
      
      (*vid->blit) (dest,x,blit_y,w,blit_h,dest,x,full_line_y,lgop);
    }
    else {
      /* Draw the line normally */
      
      for (i=-xo;i<w;i+=sw) {
	blit_x = x+i;
	blit_w = sw;
	blit_src_x = sx;
	if (i<0) {
	  blit_x = x;
	  blit_w += i;
	  blit_src_x -= i;
	}
	if (blit_x + blit_w > x + w)
	  blit_w = x + w - blit_x;
	
	(*vid->blit) (dest,blit_x,blit_y,blit_w,blit_h,src,blit_src_x,blit_src_y,lgop);
      }
      if (blit_h == sh)
	full_line_y = blit_y;
    }
  }
}

/* Scary slow blit, but necessary for dealing with unsupported lgop values
 * or other things that the fast lib can't deal with */
void def_blit(hwrbitmap dest, s16 x,s16 y,s16 w,s16 h, hwrbitmap src,
	      s16 src_x, s16 src_y, s16 lgop) {
   int i;
   s16 bw,bh;

   (*vid->bitmap_getsize)(src,&bw,&bh);

   /* Icky blit loop */
   for (;h;h--,y++,src_y++)
     for (i=0;i<w;i++)
	(*vid->pixel) (dest,x+i,y,(*vid->getpixel)(src,src_x+i,src_y),lgop);
}

/* Version of blit() that tolerates overlapping source and destination */
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
     * slices to do the horizontal equivalent of an upside-down blit. This is needed
     * for horizontal scrolling, or for vertical scrolling on rotated displays.
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

/* Move the bitmap's contents to a shared memory segment,
 * and fill out a pgshmbitmap structure
 */
g_error def_bitmap_getshm(hwrbitmap bmp, u32 uid, struct pgshmbitmap *shm) {
  struct stdbitmap *b = (struct stdbitmap *) bmp;
  u32 key;
  u32 id;
  u32 size = b->pitch * b->h + 1;  /* +1 for bpp<8 padding, as described in def_bitmap_new() */
  void *shmaddr;
  g_error e;

  /* Make sure we can free the existing bitmap data to move it to a shared
   * memory segment, if not assume it's already mapped to SHM or some device.
   */
  if (!b->freebits)
    return mkerror(PG_ERRT_BUSY,4);     /* Bitmap is already mapped */

  e = os_shm_alloc(&shmaddr, size, &id, &key, 1);
  errorcheck;
  os_shm_set_uid(id,uid);

  /* Copy over the bitmap data and delete the original */
  memcpy(shmaddr, b->bits, size);
  g_free(b->bits);
  b->freebits = 0;

  /* Now assign this SHM section to the bitmap */
  b->bits = (u8 *) shmaddr;
  b->shm_id = id;

  /* Fill in the easy information in the shmbitmap structure 
   */
  shm->shm_key     = htonl(key);
  shm->shm_length  = htonl(size);
  shm->width       = htons(b->w);
  shm->height      = htons(b->h);
  shm->bpp         = htons(b->bpp);
  shm->pitch       = htons(b->pitch);

  /* Default color space information. Detect an alpha channel
   * if this bitmap has one.
   */
  def_shm_colorspace(b->bpp,
		     size >= 4 && (*(u32*)shmaddr & PGCF_ALPHA),
		     shm);
  return success;
}

/* Default heuristics for reporting color information in SHM bitmaps */
void def_shm_colorspace(int bpp, int alpha, struct pgshmbitmap *shm) {
  if (bpp < 8) {
    /* Grayscale if less than 8bpp 
     */
    shm->format = htonl(PG_BITFORMAT_GRAYSCALE);
  }
  else if (bpp == 32 && alpha) {
    /* Is this an ARGB image? 
     */
    shm->format       = htonl(PG_BITFORMAT_TRUECOLOR | PG_BITFORMAT_ALPHA);
    shm->red_mask     = htonl(0x00FF0000);
    shm->green_mask   = htonl(0x0000FF00);
    shm->blue_mask    = htonl(0x000000FF);
    shm->alpha_mask   = htonl(0x7F000000);
    shm->red_shift    = htons(16);
    shm->green_shift  = htons(8);
    shm->alpha_shift  = htons(24);
    shm->red_length   = htons(8);
    shm->green_length = htons(8);
    shm->blue_length  = htons(8);
    shm->alpha_length = htons(7);
  }
  else if (bpp == 16) {
    /* Assume 5-6-5 color in 16bpp mode
     */
    shm->format       = htonl(PG_BITFORMAT_TRUECOLOR);
    shm->red_mask     = htonl(0x0000F800);
    shm->green_mask   = htonl(0x000007E0);
    shm->blue_mask    = htonl(0x0000001F);
    shm->red_shift    = htons(11);
    shm->green_shift  = htons(5);
    shm->red_length   = htons(5);
    shm->green_length = htons(6);
    shm->blue_length  = htons(5);
  }
  else if (bpp >= 24) {
    shm->format       = htonl(PG_BITFORMAT_TRUECOLOR);
    shm->red_mask     = htonl(0x00FF0000);
    shm->green_mask   = htonl(0x0000FF00);
    shm->blue_mask    = htonl(0x000000FF);
    shm->red_shift    = htons(16);
    shm->green_shift  = htons(8);
    shm->red_length   = htons(8);
    shm->green_length = htons(8);
    shm->blue_length  = htons(8);
  }
  else if (bpp == 8) { 
    /* A few different choices in 8bpp mode.. 
     */
#ifdef CONFIG_PAL8_222
    shm->format       = htonl(PG_BITFORMAT_TRUECOLOR);
    shm->red_mask     = htonl(0x00000030);
    shm->green_mask   = htonl(0x0000000C);
    shm->blue_mask    = htonl(0x00000003);
    shm->red_shift    = htons(4);
    shm->green_shift  = htons(2);
    shm->red_length   = htons(2);
    shm->green_length = htons(2);
    shm->blue_length  = htons(2);
#endif
#ifdef CONFIG_PAL8_233
    shm->format       = htonl(PG_BITFORMAT_TRUECOLOR);
    shm->red_mask     = htonl(0x000000C0);
    shm->green_mask   = htonl(0x00000038);
    shm->blue_mask    = htonl(0x00000007);
    shm->red_shift    = htons(6);
    shm->green_shift  = htons(3);
    shm->red_length   = htons(2);
    shm->green_length = htons(3);
    shm->blue_length  = htons(3);
#endif
#ifdef CONFIG_PAL8_CUSTOM
    shm->format       = htonl(PG_BITFORMAT_INDEXED);
#endif
  }
}

/* The End */
