/* $Id: rotate90.c,v 1.5 2001/03/20 00:46:44 micahjd Exp $
 *
 * rotate90.c - Video wrapper to rotate the screen 90 degrees
 *
 * This is crufty, but I don't know of any other way to do it that wouldn't
 * bog down the drivers when not rotated.
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
#include <pgserver/video.h>

/******* Simple wrapper functions */

void rotate90_pixel(int x,int y,hwrcolor c) {
   (*vid->pixel)(y,vid->yres-1-x,c);
}
hwrcolor rotate90_getpixel(int x,int y) {
   return (*vid->getpixel)(y,vid->yres-1-x);
}
void rotate90_addpixel(int x,int y,pgcolor c) {
   (*vid->addpixel)(y,vid->yres-1-x,c);
}
void rotate90_subpixel(int x,int y,pgcolor c) {
   (*vid->subpixel)(y,vid->yres-1-x,c);
}
void rotate90_update(int x,int y,int w,int h) {
   (*vid->update)(y,vid->yres-x-w,h,w);
}
void rotate90_rect(int x,int y,int w,int h,hwrcolor c) {
   (*vid->rect)(y,vid->yres-x-w,h,w,c);
}
void rotate90_dim(int x,int y,int w,int h) {
   (*vid->dim)(y,vid->yres-x-w,h,w);
}
void rotate90_coord_logicalize(int *x,int *y) {
   int tx = *x;
   *x = vid->yres-1-*y;
   *y = tx;
}

/******* Special-case wrapper functions */

void rotate90_gradient(int x,int y,int w,int h,int angle,
		       pgcolor c1,pgcolor c2,int translucent) {
   (*vid->gradient)(y,vid->yres-x-w,h,w,angle-90,c1,c2,translucent);
}
void rotate90_slab(int x,int y,int w,hwrcolor c) {
   (*vid->bar)(y,vid->yres-x-w,w,c);
}
void rotate90_bar(int x,int y,int h,hwrcolor c) {
   (*vid->slab)(y,vid->yres-1-x,h,c);
}
void rotate90_line(int x1,int y1,int x2,int y2,hwrcolor c) {
   (*vid->line)(y1,vid->yres-1-x1,y2,vid->yres-1-x2,c);
}
void rotate90_blit(hwrbitmap src,int src_x,int src_y,
		   int dest_x,int dest_y,int w,int h,int lgop) {
   (*vid->blit)(src,0,0,dest_y,vid->yres-dest_x-w,h,w,lgop);
}
void rotate90_unblit(int src_x,int src_y,hwrbitmap dest,int dest_x,
		     int dest_y,int w,int h) {
 //  (*vid->unblit)(src_y,vid->yres-src_x-w,dest,dest_y,dest_x,h,w);
}
void rotate90_scrollblit(int src_x,int src_y,int dest_x,int dest_y,
			 int w,int h) {
   /* FIXME */
}
void rotate90_tileblit(hwrbitmap src,int src_x,int src_y,int src_w,
		       int src_h,int dest_x,int dest_y,int dest_w,
		       int dest_h) {
   /* FIXME */
}
void rotate90_charblit(unsigned char *chardat,int dest_x,int dest_y,
		       int w,int h,int lines,hwrcolor c,
		       struct cliprect *clip) {
   struct cliprect cr;
   struct cliprect *crp;
   
   if (clip) {
      cr.x1 = clip->y1;
      cr.y1 = vid->yres-1-clip->x2;
      cr.x2 = clip->y2;
      cr.y2 = vid->yres-1-clip->x1;
      crp = &cr;
   }
   else
     crp = NULL;
   (*vid->charblit_v)(chardat,dest_y,vid->yres-1-dest_x,w,h,lines,c,crp);
}
void rotate90_charblit_v(unsigned char *chardat,int dest_x,int dest_y,
		       int w,int h,int lines,hwrcolor c,
		       struct cliprect *clip) {
   struct cliprect cr;
   struct cliprect *crp;
   
   if (clip) {
      cr.x1 = clip->y1;
      cr.y1 = vid->yres-1-clip->x2;
      cr.x2 = clip->y2;
      cr.y2 = vid->yres-1-clip->x1;
      crp = &cr;
   }
   else
     crp = NULL;
   (*vid->charblit)(chardat,dest_y,vid->yres-1-dest_x,w,h,lines,c,crp);
}

/******* Bitmap rotation */

/* Tack that rotation onto any bitmap loading */

#ifdef CONFIG_FORMAT_XBM
g_error rotate90_bitmap_loadxbm(struct stdbitmap **bmp,
				unsigned char *data,
				int w,int h,
				hwrcolor fg,
				hwrcolor bg) {
   g_error e;
   e = (*vid->bitmap_loadxbm)(bmp,data,w,h,fg,bg);
   return (*vid->bitmap_rotate90)(bmp);
}
#endif

g_error rotate90_bitmap_load(hwrbitmap *bmp,u8 *data,u32 datalen) {
   g_error e;
   e = (*vid->bitmap_load)(bmp,data,datalen);
   errorcheck;
   return (*vid->bitmap_rotate90)(bmp);
}
   
/******* Registration */

void vidwrap_rotate90(struct vidlib *vid) {
   vid->pixel = &rotate90_pixel;
   vid->getpixel = &rotate90_getpixel;
   vid->addpixel = &rotate90_addpixel;
   vid->subpixel = &rotate90_subpixel;
   vid->update = &rotate90_update;
   vid->slab = &rotate90_slab;
   vid->bar = &rotate90_bar;
   vid->line = &rotate90_line;
   vid->rect = &rotate90_rect;
   vid->gradient = &rotate90_gradient;
   vid->dim = &rotate90_dim;
   vid->blit = &rotate90_blit;
   vid->unblit = &rotate90_unblit;
   vid->scrollblit = &rotate90_scrollblit;
   vid->tileblit = &rotate90_tileblit;
   vid->charblit = &rotate90_charblit;
   vid->charblit_v = &rotate90_charblit_v;
   vid->coord_logicalize = &rotate90_coord_logicalize;
   vid->bitmap_loadxbm = &rotate90_bitmap_loadxbm;
   vid->bitmap_load = &rotate90_bitmap_load;
}

/* The End */

