/* $Id$
 *
 * rotate270.c - Video wrapper to rotate the screen 270 degrees
 *
 * This is crufty, but I don't know of any other way to do it that wouldn't
 * bog down the drivers when not rotated.
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
#include <pgserver/video.h>
#include <pgserver/appmgr.h>
#include <pgserver/render.h>

struct pgquad *rotate270_rotateclip(struct pgquad *clip) {
  static struct pgquad cr;
  if (clip) {
    cr.x1 = vid->xres - 1 - clip->y2;
    cr.y1 = clip->x1;
    cr.x2 = vid->xres - 1 - clip->y1;
    cr.y2 = clip->x2;
    return &cr;
  }
  return NULL;
}

/******* Simple wrapper functions */

void rotate270_pixel(hwrbitmap dest,s16 x,s16 y,hwrcolor c,s16 lgop) {
   s16 dx,dy;
   (*vid->bitmap_getsize)(dest,&dx,&dy);
   (*vid->pixel)(dest,dx-1-y,x,c,lgop);
}
hwrcolor rotate270_getpixel(hwrbitmap src,s16 x,s16 y) {
   s16 dx,dy;
   (*vid->bitmap_getsize)(src,&dx,&dy);
   return (*vid->getpixel)(src,dx-1-y,x);
}
void rotate270_update(hwrbitmap display,s16 x,s16 y,s16 w,s16 h) {
   (*vid->update)(display,vid->xres-y-h,x,h,w);
}
void rotate270_rect(hwrbitmap dest,s16 x,s16 y,s16 w,s16 h,
		   hwrcolor c,s16 lgop) {
   s16 dx,dy;
   (*vid->bitmap_getsize)(dest,&dx,&dy);
   (*vid->rect)(dest,dx-y-h,x,h,w,c,lgop);
}
void rotate270_blur(hwrbitmap dest,s16 x,s16 y,s16 w,s16 h,
		   s16 radius) {
   s16 dx,dy;
   (*vid->bitmap_getsize)(dest,&dx,&dy);
   (*vid->blur)(dest,dx-y-h,x,h,w,radius);
}
void rotate270_ellipse(hwrbitmap dest,s16 x,s16 y,s16 w,s16 h,
		       hwrcolor c,s16 lgop) {
   s16 dx,dy;
   (*vid->bitmap_getsize)(dest,&dx,&dy);
   (*vid->ellipse)(dest,dx-y-h,x,h,w,c,lgop);
}
void rotate270_fellipse(hwrbitmap dest,s16 x,s16 y,s16 w,s16 h,
			hwrcolor c,s16 lgop) {
   s16 dx,dy;
   (*vid->bitmap_getsize)(dest,&dx,&dy);
   (*vid->fellipse)(dest,dx-y-h,x,h,w,c,lgop);
}
void rotate270_coord_logicalize(int *x,int *y) {
   int ty = *y;
   *y = vid->xres-1-*x;
   *x = ty;
}
void rotate270_coord_physicalize(int *x,int *y) {
   int ty = *y;
   *y = *x;
   *x = vid->xres-1-ty;
}
g_error rotate270_bitmap_getshm(hwrbitmap bmp, u32 uid, struct pgshmbitmap *shm) {
  g_error e;
  e = vid->bitmap_getshm(bmp,uid,shm);
  shm->format = htonl(ntohl(shm->format) | PG_BITFORMAT_ROTATE270);
  return e;
}

/******* Special-case wrapper functions */

void rotate270_gradient(hwrbitmap dest,s16 x,s16 y,s16 w,s16 h,s16 angle,
		       pgcolor c1,pgcolor c2,s16 lgop) {
   s16 dx,dy;
   (*vid->bitmap_getsize)(dest,&dx,&dy);
   (*vid->gradient)(dest,dx-y-h,x,h,w,angle-270,c1,c2,lgop);
}
void rotate270_slab(hwrbitmap dest,s16 x,s16 y,s16 w,hwrcolor c,s16 lgop) {
   s16 dx,dy;
   (*vid->bitmap_getsize)(dest,&dx,&dy);
   (*vid->bar)(dest,dx-1-y,x,w,c,lgop);
}
void rotate270_bar(hwrbitmap dest,s16 x,s16 y,s16 h,hwrcolor c,s16 lgop) {
   s16 dx,dy;
   (*vid->bitmap_getsize)(dest,&dx,&dy);
   (*vid->slab)(dest,dx-y-h,x,h,c,lgop);
}
void rotate270_line(hwrbitmap dest,s16 x1,s16 y1,s16 x2,
		   s16 y2,hwrcolor c, s16 lgop) {
   s16 dx,dy;
   (*vid->bitmap_getsize)(dest,&dx,&dy);
   (*vid->line)(dest,dx-1-y1,x1,dx-1-y2,x2,c,lgop);
}
void rotate270_blit(hwrbitmap dest,s16 dest_x,s16 dest_y,s16 w, s16 h,
		   hwrbitmap src,s16 src_x,s16 src_y,
		   s16 lgop) {
   s16 bw,bh;
   s16 dx,dy;
   (*vid->bitmap_getsize)(dest,&dx,&dy);
   (*vid->bitmap_getsize)(src,&bw,&bh);
   (*vid->blit)(dest,dx-dest_y-h,dest_x,h,w,
		src,bw-h-src_y,src_x,lgop);
}
void rotate270_rotateblit(hwrbitmap dest, s16 dest_x, s16 dest_y,
			  hwrbitmap src, s16 src_x, s16 src_y, s16 src_w, s16 src_h,
			  struct pgquad *clip, s16 angle, s16 lgop) {
  s16 dw,dh;
  s16 bw,bh;
  vid->bitmap_getsize(dest,&dw,&dh);
  vid->bitmap_getsize(src,&bw,&bh);
  vid->rotateblit(dest,dw-dest_y-1,dest_x,
		  src,src_x,src_y,src_w,src_h,
		  rotate270_rotateclip(clip),angle,lgop);
}
void rotate270_scrollblit(hwrbitmap dest,s16 dest_x,s16 dest_y,s16 w, s16 h,
		   hwrbitmap src,s16 src_x,s16 src_y,
		   s16 lgop) {
   s16 bw,bh;
   s16 dx,dy;
   (*vid->bitmap_getsize)(dest,&dx,&dy);
   (*vid->bitmap_getsize)(src,&bw,&bh);
   (*vid->scrollblit)(dest,dx-dest_y-h,dest_x,h,w,
		      src,bw-h-src_y,src_x,lgop);
}
void rotate270_multiblit(hwrbitmap dest,s16 dest_x,s16 dest_y,
			 s16 dest_w,s16 dest_h,
			 hwrbitmap src,s16 src_x,s16 src_y,
			 s16 src_w,s16 src_h,s16 xo,s16 yo,s16 lgop) {
   s16 sw,sh,dw,dh;
   (*vid->bitmap_getsize)(dest,&dw,&dh);
   (*vid->bitmap_getsize)(src,&sw,&sh);

   /* See the explanation in rotate90_multiblit */

   (*vid->multiblit)(dest,dw-dest_y-dest_h,dest_x,dest_h,dest_w,
		     src,sw-src_y-src_h,src_x,src_h,src_w,
		     src_h - ((yo + dest_h) % src_h), xo,
		     lgop);
}

void rotate270_charblit(hwrbitmap dest,u8 *chardat,s16 dest_x,s16 dest_y,
		       s16 w,s16 h,s16 lines,s16 angle,hwrcolor c,
		       struct pgquad *clip, s16 lgop, int pitch) {
   s16 dx,dy;
   (*vid->bitmap_getsize)(dest,&dx,&dy);

   /* Rotate the text */
   angle += 270;
   angle %= 360;
   if (angle<0) angle += 360;
   
   (*vid->charblit)(dest,chardat,dx-dest_y-1,dest_x,w,h,
		    lines,angle,c,rotate270_rotateclip(clip),lgop,pitch);
}
#ifdef CONFIG_FONTENGINE_FREETYPE
void rotate270_alpha_charblit(hwrbitmap dest,u8 *chardat,s16 dest_x,s16 dest_y,
			      s16 w,s16 h,int char_pitch,u8 *gammatable,s16 angle,hwrcolor c,
			      struct pgquad *clip, s16 lgop) {
   s16 dx,dy;
   (*vid->bitmap_getsize)(dest,&dx,&dy);

   /* Rotate the text */
   angle += 270;
   angle %= 360;
   if (angle<0) angle += 360;
   
   (*vid->alpha_charblit)(dest,chardat,dx-dest_y-1,dest_x,w,h,
			  char_pitch,gammatable,angle,c,rotate270_rotateclip(clip),lgop);
}
#endif

/******* Bitmap rotation */

/* Tack that rotation onto any bitmap loading */

g_error rotate270_bitmap_new(hwrbitmap *bmp,s16 w,s16 h,u16 bpp) {
   return (*vid->bitmap_new)(bmp,h,w,bpp);
}

#ifdef CONFIG_FORMAT_XBM
g_error rotate270_bitmap_loadxbm(hwrbitmap *bmp,
				const u8 *data,
				s16 w,s16 h,
				hwrcolor fg,
				hwrcolor bg) {
   g_error e;
   e = (*vid->bitmap_loadxbm)(bmp,data,w,h,fg,bg);
   errorcheck;
   return bitmap_rotate(bmp,270);
}
#endif

g_error rotate270_bitmap_load(hwrbitmap *bmp,const u8 *data,u32 datalen) {
   g_error e;
   e = (*vid->bitmap_load)(bmp,data,datalen);
   errorcheck;
   return bitmap_rotate(bmp,270);
}

g_error rotate270_bitmap_getsize(hwrbitmap bmp,s16 *w,s16 *h) {
   return (*vid->bitmap_getsize)(bmp,h,w);
}

/* Rotate all loaded bitmaps when this mode is entered/exited */

g_error rotate270_entermode(void) {
   return bitmap_rotate_all(270);
}

g_error rotate270_exitmode(void) {
   return bitmap_rotate_all(90);
}

void rotate270_coord_keyrotate(int *k) {
  switch (*k) {
  case PGKEY_UP:    *k = PGKEY_LEFT;  break;
  case PGKEY_RIGHT: *k = PGKEY_UP;    break;
  case PGKEY_DOWN:  *k = PGKEY_RIGHT; break;
  case PGKEY_LEFT:  *k = PGKEY_DOWN;  break;
  }
}

/******* Registration */

void vidwrap_rotate270(struct vidlib *vid) {
   vid->pixel = &rotate270_pixel;
   vid->getpixel = &rotate270_getpixel;
   vid->update = &rotate270_update;
   vid->slab = &rotate270_slab;
   vid->bar = &rotate270_bar;
   vid->line = &rotate270_line;
   vid->rect = &rotate270_rect;
   vid->blur = &rotate270_blur;
   vid->ellipse = &rotate270_ellipse;
   vid->fellipse = &rotate270_fellipse;
   vid->gradient = &rotate270_gradient;
   vid->blit = &rotate270_blit;
   vid->rotateblit = &rotate270_rotateblit;
   vid->multiblit = &rotate270_multiblit;
   vid->charblit = &rotate270_charblit;
#ifdef CONFIG_FONTENGINE_FREETYPE
   vid->alpha_charblit = &rotate270_alpha_charblit;
#endif
   vid->scrollblit = &rotate270_scrollblit;
   vid->coord_logicalize = &rotate270_coord_logicalize;
   vid->coord_physicalize = &rotate270_coord_physicalize;
#ifdef CONFIG_FORMAT_XBM
   vid->bitmap_loadxbm = &rotate270_bitmap_loadxbm;
#endif
   vid->bitmap_load = &rotate270_bitmap_load;
   vid->bitmap_new = &rotate270_bitmap_new;
   vid->bitmap_getshm = &rotate270_bitmap_getshm;
   vid->bitmap_getsize = &rotate270_bitmap_getsize;
   vid->entermode = &rotate270_entermode;
   vid->exitmode = &rotate270_exitmode;
   vid->coord_keyrotate = &rotate270_coord_keyrotate;
}

/* The End */

