/* $Id: sdlmin.c,v 1.7 2000/10/10 00:33:37 micahjd Exp $
 *
 * sdlmin.c - video driver wrapper for SDL.
 *            this 'min' version defines only the
 *            minimum required functions, as a way
 *            to test the default implementations.
 *
 * Example of a pretty simple (but slow) PicoGUI video driver.
 * This implements the minimum required functions, so it runs
 * much slower than the sdl.c driver.  (Slow is a relative term,
 * of course.  I'm sure this runs faster than Enlightenment on
 * a 386..) This does have a few things that some drivers won't
 * need, like palette support and support for multiple bit depths.
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000 Micah Dowty <micahjd@users.sourceforge.net>
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

#ifdef DRIVER_SDLMIN

#include <pgserver/video.h>
#include <SDL.h>

SDL_Surface *sdlmin_vidsurf;

/******************************************** Implementations */

g_error sdlmin_init(int xres,int yres,int bpp,unsigned long flags) {
  unsigned long sdlflags = 0;
  char str[80];

  /* Default mode: 640x480 */
  if (!xres) xres = 640;
  if (!yres) yres = 480;

  /* Start up the SDL video subsystem thingy */
  if (SDL_Init(SDL_INIT_VIDEO))
    return mkerror(PG_ERRT_IO,46);

  /* Interpret flags */
  if (flags & PG_VID_FULLSCREEN)
    sdlflags |= SDL_FULLSCREEN;

  /* Set the video mode */
  sdlmin_vidsurf = SDL_SetVideoMode(xres,yres,bpp,sdlflags);
  if (!sdlmin_vidsurf)
    return mkerror(PG_ERRT_IO,47);

  /* Save the actual video mode (might be different than what
     was requested) */
  vid->xres = sdlmin_vidsurf->w;
  vid->yres = sdlmin_vidsurf->h;
  vid->bpp  = sdlmin_vidsurf->format->BitsPerPixel;

  /* If this is 8bpp (SDL doesn't support <8bpp modes) 
     set up a 2-3-3 palette for pseudo-RGB */
  if (vid->bpp==8) {
    int i;
    SDL_Color palette[256];

    for (i=0;i<256;i++) {
      palette[i].r = i & 0xC0;
      palette[i].g = (i & 0x38) << 2;
      palette[i].b = (i & 0x07) << 5;
    }
    SDL_SetColors(sdlmin_vidsurf,palette,0,256);
  }

  /* Info */
  sprintf(str,"PicoGUI (sdlmin@%dx%dx%d)",
	  vid->xres,vid->yres,vid->bpp);
  SDL_WM_SetCaption(str,NULL);

  /* Clipping off */
  (*vid->clip_off)();

  return sucess;
}

void sdlmin_close(void) {
  SDL_Quit();
}

void sdlmin_pixel(int x,int y,hwrcolor c) {
  unsigned long *p;

  if (x<vid->clip_x1 || x>vid->clip_x2 ||
      y<vid->clip_y1 || y>vid->clip_y2)
    return;

  /* SDL doesn't have a good ol' pixel function... */

  switch (vid->bpp) {
  case 8:
    ((unsigned char *)sdlmin_vidsurf->pixels)[vid->xres*y+x] = c;
    return;
  case 16:
    ((unsigned short *)sdlmin_vidsurf->pixels)[vid->xres*y+x] = c;
    return;
  case 32:
    ((unsigned long *)sdlmin_vidsurf->pixels)[vid->xres*y+x] = c;
    return;
  case 24:
    p = ((unsigned char *)sdlmin_vidsurf->pixels) + (vid->xres*y+x)*3;
    *(((unsigned char *)p)++) = (unsigned char) c;
    *(((unsigned char *)p)++) = (unsigned char) (c>>8);
    *((unsigned char *)p) = (unsigned char) (c>>16);
  }
}

hwrcolor sdlmin_getpixel(int x,int y) {
  switch (vid->bpp) {
  case 8:
    return ((unsigned char *)sdlmin_vidsurf->pixels)[vid->xres*y+x];
  case 16:
    return ((unsigned short *)sdlmin_vidsurf->pixels)[vid->xres*y+x];
  case 32:
    return ((unsigned long *)sdlmin_vidsurf->pixels)[vid->xres*y+x];
  case 24:
    return *((unsigned long *)(((unsigned char *)sdlmin_vidsurf->
				pixels) + (vid->xres*y+x)*3));
  }
}

void sdlmin_update(void) {
  /* Too lazy to keep track of which areas were changed...
     besides, this is the simple-easy-to-understand driver! */
  SDL_UpdateRect(sdlmin_vidsurf,0,0,0,0);
}

void sdlmin_blit(struct stdbitmap *src,int src_x,int src_y,
		 struct stdbitmap *dest,int dest_x,int dest_y,
		 int w,int h,int lgop) {
  struct stdbitmap screenb;

  int i,j,s_of,d_of;
  unsigned char *s,*sl,*d,*dl;

  if (lgop==PG_LGOP_NULL) return;

  if (src && (w>(src->w-src_x) || h>(src->h-src_y))) {
    int i,j;

    /* Do a tiled blit */
    for (i=0;i<w;i+=src->w)
      for (j=0;j<h;j+=src->h)
        sdlmin_blit(src,0,0,dest,dest_x+i,dest_y+j,src->w,src->h,lgop);

    return;
  }

  if ((dest_x+w-1)>vid->clip_x2) w = vid->clip_x2-dest_x+1;
  if ((dest_y+h-1)>vid->clip_y2) h = vid->clip_y2-dest_y+1;
  if (dest_x<vid->clip_x1) {
    w -= vid->clip_x1 - dest_x;
    src_x += vid->clip_x1 - dest_x;
    dest_x = vid->clip_x1;
  }
  if (dest_y<vid->clip_y1) {
    h -= vid->clip_y1 - dest_y;
    src_y += vid->clip_y1 - dest_y;
    dest_y = vid->clip_y1;
  }
  if (w<=0 || h<=0) return;

  screenb.bits = sdlmin_vidsurf->pixels;
  screenb.w    = vid->xres;
  screenb.h    = vid->yres;

  if (!src) {
    src = &screenb;
  }
  if (!dest) {
    dest = &screenb;
  }

  /* set up pointers */
  s_of = src->w * vid->bpp / 8;
  d_of = dest->w * vid->bpp / 8;
  s = src->bits + src_x*vid->bpp/8 + src_y*s_of;
  d = dest->bits + dest_x*vid->bpp/8 + dest_y*d_of;
  w = w*vid->bpp/8;

  /* Now the actual blitter code depends on the LGOP */
  switch (lgop) {
    
  case PG_LGOP_NONE:
    for (;h;h--,s+=s_of,d+=d_of)
      memcpy(d,s,w);
    break;
    
  case PG_LGOP_OR:
    for (sl=s,dl=d;h;h--,s=(sl+=s_of),d=(dl+=d_of))
      for (i=w;i;i--,d++,s++)
        *d |= *s;
    break;
    
  case PG_LGOP_AND:
    for (sl=s,dl=d;h;h--,s=(sl+=s_of),d=(dl+=d_of))
      for (i=w;i;i--,d++,s++)
        *d &= *s;
    break;
    
  case PG_LGOP_XOR:
    for (sl=s,dl=d;h;h--,s=(sl+=s_of),d=(dl+=d_of))
      for (i=w;i;i--,d++,s++)
        *d ^= *s;
    break;
    
  case PG_LGOP_INVERT:
    for (sl=s,dl=d;h;h--,s=(sl+=s_of),d=(dl+=d_of))
      for (i=w;i;i--,d++,s++)
        *d = (*s) ^ 0xFFFFFF;
    break;
    
  case PG_LGOP_INVERT_OR:
    for (sl=s,dl=d;h;h--,s=(sl+=s_of),d=(dl+=d_of))
      for (i=w;i;i--,d++,s++)
        *d |= (*s) ^ 0xFFFFFF;
    break;
    
  case PG_LGOP_INVERT_AND:
    for (sl=s,dl=d;h;h--,s=(sl+=s_of),d=(dl+=d_of))
      for (i=w;i;i--,d++,s++)
        *d &= (*s) ^ 0xFFFFFF;
    break;
    
  case PG_LGOP_INVERT_XOR:
    for (sl=s,dl=d;h;h--,s=(sl+=s_of),d=(dl+=d_of))
      for (i=w;i;i--,d++,s++)
        *d ^= (*s) ^ 0xFFFFFF;
    break;
  }
}

void sdlmin_clip_set(int x1,int y1,int x2,int y2) {
  SDL_SetClipping(sdlmin_vidsurf,y1,x1,y2,x2);
  vid->clip_x1 = x1;
  vid->clip_y1 = y1;
  vid->clip_x2 = x2;
  vid->clip_y2 = y2;
}

/******************************************** Driver registration */

/* This func. is passed to registervid */
g_error sdlmin_regfunc(struct vidlib *v) {
  v->init = &sdlmin_init;
  v->close = &sdlmin_close;   /* Not strictly required (raw framebuffers
				 might not need it) but SDL needs this */
  v->pixel = &sdlmin_pixel;
  v->getpixel = &sdlmin_getpixel;
  v->update = &sdlmin_update; /* Again not required, but good for SDL */
  v->blit = &sdlmin_blit;
  v->clip_set = &sdlmin_clip_set; /* If the underlying driver supports it,
				     go for it. Simplifies this driver a lot. */

  return sucess;
}

#endif /* DRIVER_SDLMIN */
/* The End */
