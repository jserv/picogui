/* $Id$
 *
 * rotate90.c - Video wrapper to rotate the screen 90 degrees
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

struct pgquad *rotate90_rotateclip(struct pgquad *clip) {
  static struct pgquad cr;
  if (clip) {
    cr.x1 = clip->y1;
    cr.y1 = vid->yres-1-clip->x2;
    cr.x2 = clip->y2;
    cr.y2 = vid->yres-1-clip->x1;
    return &cr;
  }
  return NULL;
}
 
/******* Simple wrapper functions */

void rotate90_pixel(hwrbitmap dest,s16 x,s16 y,hwrcolor c,s16 lgop) {
   s16 dx,dy;
   (*vid->bitmap_getsize)(dest,&dx,&dy);
   (*vid->pixel)(dest,y,dy-1-x,c,lgop);
}
hwrcolor rotate90_getpixel(hwrbitmap src,s16 x,s16 y) {
   s16 dx,dy;
   (*vid->bitmap_getsize)(src,&dx,&dy);
   return (*vid->getpixel)(src,y,dy-1-x);
}
void rotate90_update(hwrbitmap display,s16 x,s16 y,s16 w,s16 h) {
   (*vid->update)(display,y,vid->yres-x-w,h,w);
}
void rotate90_rect(hwrbitmap dest,s16 x,s16 y,s16 w,s16 h,
		   hwrcolor c,s16 lgop) {
   s16 dx,dy;
   (*vid->bitmap_getsize)(dest,&dx,&dy);
   (*vid->rect)(dest,y,dy-x-w,h,w,c,lgop);
}
void rotate90_blur(hwrbitmap dest,s16 x,s16 y,s16 w,s16 h,
		   s16 radius) {
   s16 dx,dy;
   (*vid->bitmap_getsize)(dest,&dx,&dy);
   (*vid->blur)(dest,y,dy-x-w,h,w,radius);
}
void rotate90_ellipse(hwrbitmap dest,s16 x,s16 y,s16 w,s16 h,
		      hwrcolor c,s16 lgop) {
   s16 dx,dy;
   (*vid->bitmap_getsize)(dest,&dx,&dy);
   (*vid->ellipse)(dest,y,dy-x-w,h,w,c,lgop);
}
void rotate90_fellipse(hwrbitmap dest,s16 x,s16 y,s16 w,s16 h,
		       hwrcolor c,s16 lgop) {
   s16 dx,dy;
   (*vid->bitmap_getsize)(dest,&dx,&dy);
   (*vid->fellipse)(dest,y,dy-x-w,h,w,c,lgop);
}
void rotate90_coord_logicalize(int *x,int *y) {
   int tx = *x;
   *x = vid->yres-1-*y;
   *y = tx;
}
void rotate90_coord_physicalize(int *x,int *y) {
   int tx = *x;
   *x = *y;
   *y = vid->yres-1-tx;
}
g_error rotate90_bitmap_getshm(hwrbitmap bmp, u32 uid, struct pgshmbitmap *shm) {
  g_error e;
  e = vid->bitmap_getshm(bmp,uid,shm);
  shm->format = htonl(ntohl(shm->format) | PG_BITFORMAT_ROTATE90);
  return e;
}

/******* Special-case wrapper functions */

void rotate90_gradient(hwrbitmap dest,s16 x,s16 y,s16 w,s16 h,s16 angle,
		       pgcolor c1,pgcolor c2,s16 lgop) {
   s16 dx,dy;
   (*vid->bitmap_getsize)(dest,&dx,&dy);
   (*vid->gradient)(dest,y,dy-x-w,h,w,angle-90,c1,c2,lgop);
}
void rotate90_slab(hwrbitmap dest,s16 x,s16 y,s16 w,hwrcolor c,s16 lgop) {
   s16 dx,dy;
   (*vid->bitmap_getsize)(dest,&dx,&dy);
   (*vid->bar)(dest,y,dy-x-w,w,c,lgop);
}
void rotate90_bar(hwrbitmap dest,s16 x,s16 y,s16 h,hwrcolor c,s16 lgop) {
   s16 dx,dy;
   (*vid->bitmap_getsize)(dest,&dx,&dy);
   (*vid->slab)(dest,y,dy-1-x,h,c,lgop);
}
void rotate90_line(hwrbitmap dest,s16 x1,s16 y1,s16 x2,
		   s16 y2,hwrcolor c, s16 lgop) {
   s16 dx,dy;
   (*vid->bitmap_getsize)(dest,&dx,&dy);
   (*vid->line)(dest,y1,dy-1-x1,y2,dy-1-x2,c,lgop);
}
void rotate90_blit(hwrbitmap dest,s16 dest_x,s16 dest_y,s16 w, s16 h,
		   hwrbitmap src,s16 src_x,s16 src_y,
		   s16 lgop) {
   s16 bw,bh;
   s16 dx,dy;
   (*vid->bitmap_getsize)(dest,&dx,&dy);
   (*vid->bitmap_getsize)(src,&bw,&bh);

   (*vid->blit)(dest,dest_y,dy-dest_x-w,h,w,
		src,src_y,bh-src_x-w,lgop);
}
void rotate90_rotateblit(hwrbitmap dest, s16 dest_x, s16 dest_y,
			 hwrbitmap src, s16 src_x, s16 src_y, s16 src_w, s16 src_h,
			 struct pgquad *clip, s16 angle, s16 lgop) {
  s16 dw,dh;
  s16 bw,bh;
  vid->bitmap_getsize(dest,&dw,&dh);
  vid->bitmap_getsize(src,&bw,&bh);
  vid->rotateblit(dest,dest_y,dh-dest_x-1,
		  src,src_x,src_y,src_w,src_h,
		  rotate90_rotateclip(clip),angle,lgop);
}
void rotate90_scrollblit(hwrbitmap dest,s16 dest_x,s16 dest_y,s16 w, s16 h,
			 hwrbitmap src,s16 src_x,s16 src_y,
			 s16 lgop) {
   s16 bw,foo;
   s16 dx,dy;
   (*vid->bitmap_getsize)(dest,&dx,&dy);
   (*vid->bitmap_getsize)(src,&foo,&bw);

   (*vid->scrollblit)(dest,dest_y,dy-dest_x-w,h,w,
		src,src_y,bw-src_x-w,lgop);
}
void rotate90_multiblit(hwrbitmap dest,s16 dest_x,s16 dest_y,
		       s16 dest_w,s16 dest_h,
		       hwrbitmap src,s16 src_x,s16 src_y,
		       s16 src_w,s16 src_h,s16 xo,s16 yo,s16 lgop) {
   s16 bw,bh;
   s16 dx,dy;
   (*vid->bitmap_getsize)(dest,&dx,&dy);
   (*vid->bitmap_getsize)(src,&bh,&bw);
   
   /* It makes it easier to think about this if dest_* is a clipping
    * rectangle and (dest_x - xo, dest_y - yo) is an anchor for the top-left
    * of the original source rectangle. The src_* and dest_* rectangles get
    * transformed just like a normal rectangle, but we have to calculate a new
    * xo,yo relative to the original xo,yo and to the two clipping rectangles.
    *
    * This shows the destination rectangle in the middle, overlapping a set of
    * tiled source rectangles.
    *
    *  Hardware coordinates           Logical coordinates
    *
    *  +---> X                        X      
    *  |                                     
    *  |                              ^
    *  v                              |      
    *                                 |
    *  Y                              +---> Y
    *
    * 
    *  +----------+-----~----+----------+
    *  |F         |yo'       |          |
    *  |      A   |          |       B  |
    *  |       +--------~-----------+   |
    *  |       |  |          |      |   |
    *  |       |  |          |      |   |
    *  |  xo'  |  |          |      |   |
    *  +-------|--+-----~----+------|---+
    *  |       |  |          |      |   |
    *  |       |  |          |      |   |
    *  <       <  <          <      <   <
    *  >       >  >          >      >   >
    *  |       |  |          |      |   |
    *  |  yo   |  |          |      |   |
    *  +-------|--+-----~----+------|---+
    *  |       |  |          |      |   |
    *  |       +--------~-----------+   |
    *  |      D   |          |       C  |
    *  |          |xo        |          |
    *  |          |          |          |
    *  |E         |          |          |
    *  +----------+-----~----+----------+
    *
    *  The distance between E and F will be an integer multiple of src_w
    *  This distance will equal xo + dest_w + yo'
    *  xo' = yo
    *  yo' = src_w - ((xo + dest_w) % src_w)
    *
    */

   (*vid->multiblit)(dest, dest_y, dy-dest_x-dest_w, dest_h, dest_w,
		     src, src_y, bw-src_w-src_x, src_h, src_w,
		     yo, src_w - ((xo + dest_w) % src_w),
		     lgop);
}

void rotate90_charblit(hwrbitmap dest,u8 *chardat,s16 dest_x,s16 dest_y,
		       s16 w,s16 h,s16 lines,s16 angle,hwrcolor c,
		       struct pgquad *clip, s16 lgop, int pitch) {
   s16 dx,dy;
   (*vid->bitmap_getsize)(dest,&dx,&dy);
   
   /* Rotate the text */
   angle += 90;
   angle %= 360;
   if (angle<0) angle += 360;
   
   (*vid->charblit)(dest,chardat,dest_y,dy-1-dest_x,w,h,
		    lines,angle,c,rotate90_rotateclip(clip),lgop,pitch);
}

#ifdef CONFIG_FONTENGINE_FREETYPE
void rotate90_alpha_charblit(hwrbitmap dest,u8 *chardat, s16 dest_x,s16 dest_y,s16 w,s16 h,
			     int char_pitch, u8 *gammatable, s16 angle,hwrcolor c,
			     struct pgquad *clip, s16 lgop) {
   s16 dx,dy;
   (*vid->bitmap_getsize)(dest,&dx,&dy);
   
   /* Rotate the text */
   angle += 90;
   angle %= 360;
   if (angle<0) angle += 360;
   
   (*vid->alpha_charblit)(dest,chardat,dest_y,dy-1-dest_x,w,h,
			  char_pitch,gammatable,angle,c,rotate90_rotateclip(clip),lgop);
}
#endif

/******* Bitmap rotation */

/* Tack that rotation onto any bitmap loading */

g_error rotate90_bitmap_new(hwrbitmap *bmp,s16 w,s16 h,u16 bpp) {
   return (*vid->bitmap_new)(bmp,h,w,bpp);
}

#ifdef CONFIG_FORMAT_XBM
g_error rotate90_bitmap_loadxbm(hwrbitmap *bmp,
				const u8 *data,
				s16 w,s16 h,
				hwrcolor fg,
				hwrcolor bg) {
   g_error e;
   e = (*vid->bitmap_loadxbm)(bmp,data,w,h,fg,bg);
   errorcheck;
   return bitmap_rotate(bmp,90);
}
#endif

g_error rotate90_bitmap_load(hwrbitmap *bmp,const u8 *data,u32 datalen) {
   g_error e;
   e = (*vid->bitmap_load)(bmp,data,datalen);
   errorcheck;
   return bitmap_rotate(bmp,90);
}

g_error rotate90_bitmap_getsize(hwrbitmap bmp,s16 *w,s16 *h) {
   return (*vid->bitmap_getsize)(bmp,h,w);
}

/* Rotate all loaded bitmaps when this mode is entered/exited */

g_error rotate90_entermode(void) {
   return bitmap_rotate_all(90);
}

g_error rotate90_exitmode(void) {
   return bitmap_rotate_all(270);
}

void rotate90_coord_keyrotate(int *k) {
  switch (*k) {
  case PGKEY_UP:    *k = PGKEY_RIGHT; break;
  case PGKEY_RIGHT: *k = PGKEY_DOWN;  break;
  case PGKEY_DOWN:  *k = PGKEY_LEFT;  break;
  case PGKEY_LEFT:  *k = PGKEY_UP;    break;
  }
}

/******* Registration */

void vidwrap_rotate90(struct vidlib *vid) {
   vid->pixel = &rotate90_pixel;
   vid->getpixel = &rotate90_getpixel;
   vid->update = &rotate90_update;
   vid->slab = &rotate90_slab;
   vid->bar = &rotate90_bar;
   vid->line = &rotate90_line;
   vid->rect = &rotate90_rect;
   vid->blur = &rotate90_blur;
   vid->ellipse = &rotate90_ellipse;
   vid->fellipse = &rotate90_fellipse;
   vid->gradient = &rotate90_gradient;
   vid->blit = &rotate90_blit;
   vid->rotateblit = &rotate90_rotateblit;
   vid->scrollblit = &rotate90_scrollblit;
   vid->multiblit = &rotate90_multiblit;
   vid->charblit = &rotate90_charblit;
#ifdef CONFIG_FONTENGINE_FREETYPE
   vid->alpha_charblit = &rotate90_alpha_charblit;
#endif
   vid->coord_logicalize = &rotate90_coord_logicalize;
   vid->coord_physicalize = &rotate90_coord_physicalize;
#ifdef CONFIG_FORMAT_XBM
   vid->bitmap_loadxbm = &rotate90_bitmap_loadxbm;
#endif
   vid->bitmap_load = &rotate90_bitmap_load;
   vid->bitmap_new = &rotate90_bitmap_new;
   vid->bitmap_getsize = &rotate90_bitmap_getsize;
   vid->bitmap_getshm = &rotate90_bitmap_getshm;
   vid->entermode = &rotate90_entermode;
   vid->exitmode = &rotate90_exitmode;
   vid->coord_keyrotate = &rotate90_coord_keyrotate;
}

/* The End */

