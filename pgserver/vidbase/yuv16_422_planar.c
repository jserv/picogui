/* $Id: yuv16_422_planar.c,v 1.1 2003/01/20 17:12:21 thierrythevoz Exp $
 *
 * Video Base Library:
 * yuv16_422_planar.c - For 16bpp YUV 422 planar framebuffer
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
 * Thierry Thévoz <thierry.thevoz@smartdata.ch>
 * 
 */

#include <pgserver/common.h>

#include <pgserver/inlstring.h>    /* inline-assembly __memcpy */
#include <pgserver/video.h>

# include "yuv.h"
#include <stdio.h>
#include <unistd.h>


static size_t yuv16_422_planar_y_plane_size  = 0;

/* Macros to easily access the framebuffer bitmap */
#define FB_MEM     (vid->display->bits)
#define FB_BPL     (vid->display->pitch)
  
static inline int yuv16_422_planar_is_offscreen(u8* bits)
{
  return (bits != FB_MEM);
}

/************************************************** RGB shadow buffer */

/* The RGB shadow buffer.
 *
 * Note: we are not using the double buffer, because it is working
 * on an update() base, and we want to buffer on a pixel() base.
 * Our shadow buffer is here for getpixel() to avoid having to do
 * a YUV->RGB conversion.
 */
static hwrcolor* yuv16_rgb_shadow_buffer = 0;

/************************************************** Color conversion */

/* Our own conversion routines for YUV 16
 */
pgcolor yuv16_422_planar_color_hwrtopg(hwrcolor c)
{
  /* hwrcolor is in RGB until very hardware access */
  return c;
}

hwrcolor yuv16_422_planar_color_pgtohwr(pgcolor c)
{
  /* hwrcolor is in RGB until very hardware access */
  return c;
}

/************************************************** Minimum functionality */

hwrcolor yuv16_422_planar_getpixel(hwrbitmap src, s16 x, s16 y)
{
  struct stdbitmap *srcbit = (struct stdbitmap *)src;

  if( yuv16_422_planar_is_offscreen(srcbit->bits) ) {
    /* we are offscreen */
    return def_getpixel(src, x, y);
  }
  else {
    size_t offset = (vid->display->pitch)*y + x;
    return yuv16_rgb_shadow_buffer[offset];
  }
}

void yuv16_422_planar_pixel(hwrbitmap dest,
			      s16 x, s16 y, hwrcolor c,
			      s16 lgop)
{
  struct stdbitmap *dstbit = (struct stdbitmap *) dest;

  if (yuv16_422_planar_is_offscreen(dstbit->bits)) {
    /* we are offscreen */
    def_pixel(dest, x, y, c, lgop);
    return;
  }
  else {
    unsigned long r = getred(c);
    unsigned long g = getgreen(c);
    unsigned long b = getblue(c);

    size_t offset = (dstbit->pitch)*y + x;
    
    u8 *dst_y  = dstbit->bits + offset;
    u8 *dst_uv = dstbit->bits + yuv16_422_planar_y_plane_size + (offset&~1);

    switch (lgop) {      
    case PG_LGOP_NONE: {
      int y, cb, cr;
      
      yuv16_rgb_shadow_buffer[offset] = c;
      rgb_to_ycbcr(r, g, b, &y, &cb, &cr);
      *dst_y = (char)y;
      *dst_uv++ = (char)cb;
      *dst_uv = (char)cr;
      
      break; 
    }
    
    default:
    /* Not supported yet */
      return;
    }
  }
}

/*********************************************** Accelerated primitives */

/* Blit function */
void yuv16_422_planar_blit(hwrbitmap dest,
		   s16 dst_x, s16 dst_y,s16 w, s16 h,
		   hwrbitmap sbit,s16 src_x,s16 src_y,
		   s16 lgop) {
         
  struct stdbitmap *dstbit = (struct stdbitmap *) dest;
  struct stdbitmap *srcbit = (struct stdbitmap *) sbit;
  int dst_offscreen = 
         yuv16_422_planar_is_offscreen(dstbit->bits);
  int src_offscreen = 
         yuv16_422_planar_is_offscreen(srcbit->bits);
  size_t dst_dx = dst_x%2;
  size_t src_dx = src_x%2;
  size_t dw = w%2;
  
  unsigned char *dst;
  unsigned char *src;

  /* Only provides fast blit within the same framebuffer,
   * Check that we blit YUV -> YUV.
   */
  if ( dst_offscreen || src_offscreen ) {
     def_blit(dest,dst_x,dst_y,w,h,sbit,src_x,src_y,lgop);
     return;
  }
     
  /* We only support LGOP_NONE for now */
  switch (lgop) {
  case PG_LGOP_NONE: 
    /* Blit in YUV buffer */
    dst = dstbit->bits + dst_y*dstbit->pitch + dst_x;
    src = srcbit->bits + src_y*srcbit->pitch + src_x;
    if ((dst_dx==1) || (src_dx==1) || (dw==1)) {
      dw = 1;
    }
    for (;h;h--,src+=srcbit->pitch,dst+=dstbit->pitch) {
      memmove(dst,src,w); /* Y plane */
      memmove(dst-dst_dx+yuv16_422_planar_y_plane_size,
            src-src_dx+yuv16_422_planar_y_plane_size,w+dw);
    }
    /* Also blit in shadow buffer */
    dst = ((unsigned char *)yuv16_rgb_shadow_buffer) + (dst_x<<2) 
          + ((dst_y*dstbit->pitch)<<2);
    src = ((unsigned char *)yuv16_rgb_shadow_buffer) + (src_x<<2) 
          + ((src_y*srcbit->pitch)<<2);
    for (;h;h--,src+=(srcbit->pitch<<2),dst+=(dstbit->pitch<<2)) {
      memmove(dst,src,w<<2);
    }
    
    break;
    
  default:
    def_blit(dest,dst_x,dst_y,w,h,sbit,src_x,src_y,lgop);
  }
}

/*********************************************** Registration */

/* Load our driver functions into a vidlib */
void setvbl_yuv16_422_planar(struct vidlib *vid) {
  /* Call parent function */
  setvbl_default(vid);
   
  /* Overload with our own stuff */
  yuv16_422_planar_y_plane_size  = vid->xres * vid->yres;
  
  /* So that we have 32 bits RGB offscreen bitmaps */
  vid->bpp = 32;
  
  vid->color_hwrtopg = &yuv16_422_planar_color_hwrtopg;
  vid->color_pgtohwr = &yuv16_422_planar_color_pgtohwr;
  
  vid->pixel         = &yuv16_422_planar_pixel;
  vid->getpixel      = &yuv16_422_planar_getpixel;
  vid->blit          = &yuv16_422_planar_blit;
}

void yuv16_422_planar_close (void) { 
  /* Free our own stuff */
  if (yuv16_rgb_shadow_buffer) {
    free (yuv16_rgb_shadow_buffer);
  }

  /* Call parent function */
  fbdev_close();
}

g_error yuv16_422_planar_init(void) {
  g_error e;

  /* Call parent function */
  /* NOTE : The fbdev_init() function includes some specific settings
   * for yuv16_422_planar mode. See CONFIG_FB_YUV16_422_PLANAR in this.
   */
  e = fbdev_init();
  errorcheck;
   
  /* Init our own stuff */
  /* Create an RGB shadow buffer */
  yuv16_rgb_shadow_buffer = malloc(vid->xres * vid->yres * sizeof(hwrcolor));
  if (yuv16_rgb_shadow_buffer == 0) {
    yuv16_422_planar_close();
    return mkerror(PG_ERRT_MEMORY, 25); /* No mem for RGB shadow buffer */
  }
  
  return success;
}

g_error yuv16_422_planar_regfunc (struct vidlib *v) {
  /* NOTE : This function is called from parent function
   * fbdev_regfunc()
   */
  
  /* Overload with our own stuff */
  v->init = &yuv16_422_planar_init;
  v->close = &yuv16_422_planar_close;

  return success;
}

/* The End */
