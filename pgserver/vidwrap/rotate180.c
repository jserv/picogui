/* $Id: rotate180.c,v 1.9 2002/07/03 22:03:32 micahjd Exp $
 *
 * rotate180.c - Video wrapper to rotate the screen 180 degrees
 *
 * Same idea as rotate90, but a different transformation.
 * To rotate bitmaps, this runs rotate90 twice. Shouldn't slow things
 * down too much, but if it is there's an easy solution.
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
 * 
 */

#include <pgserver/common.h>
#include <pgserver/video.h>
#include <pgserver/appmgr.h>
#include <pgserver/render.h>

/******* Simple wrapper functions */

void rotate180_pixel(hwrbitmap dest,s16 x,s16 y,hwrcolor c,s16 lgop) {
   s16 dx,dy;
   (*vid->bitmap_getsize)(dest,&dx,&dy);
   (*vid->pixel)(dest,dx-1-x,dy-1-y,c,lgop);
}
hwrcolor rotate180_getpixel(hwrbitmap src,s16 x,s16 y) {
   s16 dx,dy;
   (*vid->bitmap_getsize)(src,&dx,&dy);
   return (*vid->getpixel)(src,dx-1-x,dy-1-y);
}
void rotate180_update(s16 x,s16 y,s16 w,s16 h) {
   (*vid->update)(vid->xres-x-w,vid->yres-y-h,w,h);
}
void rotate180_rect(hwrbitmap dest,s16 x,s16 y,s16 w,s16 h,
		   hwrcolor c,s16 lgop) {
   s16 dx,dy;
   (*vid->bitmap_getsize)(dest,&dx,&dy);
   (*vid->rect)(dest,dx-x-w,dy-y-h,w,h,c,lgop);
}
void rotate180_ellipse(hwrbitmap dest,s16 x,s16 y,s16 w,s16 h,
		       hwrcolor c,s16 lgop) {
   s16 dx,dy;
   (*vid->bitmap_getsize)(dest,&dx,&dy);
   (*vid->ellipse)(dest,dx-x-w,dy-y-h,w,h,c,lgop);
}
void rotate180_fellipse(hwrbitmap dest,s16 x,s16 y,s16 w,s16 h,
			hwrcolor c,s16 lgop) {
   s16 dx,dy;
   (*vid->bitmap_getsize)(dest,&dx,&dy);
   (*vid->fellipse)(dest,dx-x-w,dy-y-h,w,h,c,lgop);
}
void rotate180_coord_logicalize(int *x,int *y) {
   *x = vid->xres-1-*x;
   *y = vid->yres-1-*y;
}

/******* Special-case wrapper functions */

void rotate180_gradient(hwrbitmap dest,s16 x,s16 y,s16 w,s16 h,s16 angle,
		       pgcolor c1,pgcolor c2,s16 lgop) {
   s16 dx,dy;
   (*vid->bitmap_getsize)(dest,&dx,&dy);
   (*vid->gradient)(dest,dx-x-w,dy-y-h,w,h,angle-180,c1,c2,lgop);
}
void rotate180_slab(hwrbitmap dest,s16 x,s16 y,s16 w,hwrcolor c,s16 lgop) {
   s16 dx,dy;
   (*vid->bitmap_getsize)(dest,&dx,&dy);
   (*vid->slab)(dest,dx-x-w,dy-y-1,w,c,lgop);
}
void rotate180_bar(hwrbitmap dest,s16 x,s16 y,s16 h,hwrcolor c,s16 lgop) {
   s16 dx,dy;
   (*vid->bitmap_getsize)(dest,&dx,&dy);
   (*vid->bar)(dest,dx-x-1,dy-y-h,h,c,lgop);
}
void rotate180_line(hwrbitmap dest,s16 x1,s16 y1,s16 x2,
		    s16 y2,hwrcolor c, s16 lgop) {
   s16 dx,dy;
   (*vid->bitmap_getsize)(dest,&dx,&dy);
   (*vid->line)(dest,dx-1-x1,dy-1-y1,dx-1-x2,dy-1-y2,c,lgop);
}
void rotate180_blit(hwrbitmap dest,s16 dest_x,s16 dest_y,s16 w, s16 h,
		    hwrbitmap src,s16 src_x,s16 src_y,
		    s16 lgop) {
   s16 bw,bh,sx2,sy2;
   s16 dx,dy;
   (*vid->bitmap_getsize)(dest,&dx,&dy);
   (*vid->bitmap_getsize)(src,&bw,&bh);

   /* Avoid divide by zero in case of an empty source bitmap */
   if (!(bw && bh))
     return;
   
   sx2 = bw-(w%bw)-src_x;
   sy2 = bh-(h%bh)-src_y;
   
   (*vid->blit)(dest,dx-dest_x-w,dy-dest_y-h,w,h,
		src,sx2,sy2,lgop);
}
void rotate180_tileblit(hwrbitmap dest,s16 dest_x,s16 dest_y,
			s16 dest_w,s16 dest_h,
			hwrbitmap src,s16 src_x,s16 src_y,
			s16 src_w,s16 src_h,s16 lgop) {
   s16 bw,bh;
   s16 dx,dy;
   (*vid->bitmap_getsize)(dest,&dx,&dy);
   (*vid->bitmap_getsize)(src,&bw,&bh);
   (*vid->tileblit)(dest,dx-dest_x-dest_w,dy-dest_y-dest_h,dest_w,dest_h,
		    src,bw-src_w-src_x,bh-src_h-src_y,src_w,src_h,
		    lgop);
}
void rotate180_charblit(hwrbitmap dest,u8 *chardat,s16 dest_x,s16 dest_y,
		       s16 w,s16 h,s16 lines,s16 angle,hwrcolor c,
		       struct quad *clip,s16 lgop) {
   struct quad cr;
   struct quad *crp;
   s16 dx,dy;
   (*vid->bitmap_getsize)(dest,&dx,&dy);
   
   if (clip) {
      cr.x1 = vid->xres-1-clip->x2;
      cr.y1 = vid->yres-1-clip->y2;
      cr.x2 = vid->xres-1-clip->x1;
      cr.y2 = vid->yres-1-clip->y1;
      crp = &cr;
   }
   else
     crp = NULL;
   
   /* Rotate the text */
   angle += 180;
   angle %= 360;
   if (angle<0) angle += 360;
   
   (*vid->charblit)(dest,chardat,dx-1-dest_x,dy-1-dest_y,w,h,
		    lines,angle,c,crp,lgop);
}

/******* Bitmap rotation */

/* Tack that rotation onto any bitmap loading */

#ifdef CONFIG_FORMAT_XBM
g_error rotate180_bitmap_loadxbm(hwrbitmap *bmp,
				const u8 *data,
				s16 w,s16 h,
				hwrcolor fg,
				hwrcolor bg) {
   g_error e;
   e = (*vid->bitmap_loadxbm)(bmp,data,w,h,fg,bg);
   errorcheck;
   e = (*vid->bitmap_rotate90)(bmp);
   errorcheck;
   return (*vid->bitmap_rotate90)(bmp);
}
#endif

g_error rotate180_bitmap_load(hwrbitmap *bmp,const u8 *data,u32 datalen) {
   g_error e;
   e = (*vid->bitmap_load)(bmp,data,datalen);
   errorcheck;
   e = (*vid->bitmap_rotate90)(bmp);
   errorcheck;
   return (*vid->bitmap_rotate90)(bmp);
}

/* Rotate all loaded bitmaps when this mode is entered/exited */

g_error rotate180_entermode(void) {
   g_error e;
   e = bitmap_iterate(vid->bitmap_rotate90);
   errorcheck;
   return bitmap_iterate(vid->bitmap_rotate90);
}

void rotate180_coord_keyrotate(s16 *k) {
  switch (*k) {
  case PGKEY_UP:    *k = PGKEY_DOWN;  break;
  case PGKEY_RIGHT: *k = PGKEY_LEFT;  break;
  case PGKEY_DOWN:  *k = PGKEY_UP;    break;
  case PGKEY_LEFT:  *k = PGKEY_RIGHT; break;
  }
}

/******* Registration */

void vidwrap_rotate180(struct vidlib *vid) {
   vid->pixel = &rotate180_pixel;
   vid->getpixel = &rotate180_getpixel;
   vid->update = &rotate180_update;
   vid->slab = &rotate180_slab;
   vid->bar = &rotate180_bar;
   vid->line = &rotate180_line;
   vid->rect = &rotate180_rect;
   vid->ellipse = &rotate180_ellipse;
   vid->fellipse = &rotate180_fellipse;
   vid->gradient = &rotate180_gradient;
   vid->blit = &rotate180_blit;
   vid->tileblit = &rotate180_tileblit;
   vid->charblit = &rotate180_charblit;
   vid->coord_logicalize = &rotate180_coord_logicalize;
   vid->coord_physicalize = &rotate180_coord_logicalize;
#ifdef CONFIG_FORMAT_XBM
   vid->bitmap_loadxbm = &rotate180_bitmap_loadxbm;
#endif
   vid->bitmap_load = &rotate180_bitmap_load;
   vid->entermode = &rotate180_entermode;
   vid->exitmode = &rotate180_entermode;   /* rotation is reversible */
   vid->coord_keyrotate = &rotate180_coord_keyrotate;
}

/* The End */

