/* $Id: sdl.c,v 1.4 2000/09/03 19:27:59 micahjd Exp $
 *
 * sdl.c - video driver wrapper for SDL.
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000 Micah Dowty <micah@homesoftware.com>
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

#include <pgserver/video.h>
#include <pgserver/input.h>

#include <SDL.h>

SDL_Surface *sdl_vidsurf;
int sdl_upd_x;
int sdl_upd_y;
int sdl_upd_w;
int sdl_upd_h;

/******************************************** Utils */

/* Assimilates the given area into the update rectangle */
void sdl_addarea(int x,int y,int w,int h) {
  if (sdl_upd_w) {
    if (x < sdl_upd_x) {
      sdl_upd_w += sdl_upd_x - x;
      sdl_upd_x = x;
    }
    if (y < sdl_upd_y) {
      sdl_upd_h += sdl_upd_y - y;
      sdl_upd_y = y;
    }
    if ((w+x) > (sdl_upd_x+sdl_upd_w))
      sdl_upd_w = w+x-sdl_upd_x;
    if ((h+y) > (sdl_upd_y+sdl_upd_h))
      sdl_upd_h = h+y-sdl_upd_y;
  }
  else {
    sdl_upd_x = x;
    sdl_upd_y = y;
    sdl_upd_w = w;
    sdl_upd_h = h;
  }
}

/******************************************** Implementations */

g_error sdl_init(int xres,int yres,int bpp,unsigned long flags) {
  unsigned long sdlflags = 0;
  char str[80];

  /* Start up the SDL video subsystem thingy */
  if (SDL_Init(SDL_INIT_VIDEO))
    return mkerror(ERRT_IO,46);

  /* Interpret flags */
  if (flags & PGVID_FULLSCREEN)
    sdlflags |= SDL_FULLSCREEN;

  /* Set the video mode */
  sdl_vidsurf = SDL_SetVideoMode(xres,yres,bpp,sdlflags);
  if (!sdl_vidsurf)
    return mkerror(ERRT_IO,47);

  /* Save the actual video mode (might be different than what
     was requested) */
  vid->xres = sdl_vidsurf->w;
  vid->yres = sdl_vidsurf->h;
  vid->bpp  = sdl_vidsurf->format->BitsPerPixel;

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
    SDL_SetColors(sdl_vidsurf,palette,0,256);
  }

  /* Info */
  sprintf(str,"PicoGUI (sdl@%dx%dx%d)",
	  vid->xres,vid->yres,vid->bpp);
  SDL_WM_SetCaption(str,NULL);

  /* Clipping off */
  (*vid->clip_off)();

  /* Load a main input driver */
  return load_inlib(&sdlinput_regfunc,&inlib_main);
}

void sdl_close(void) {
  /* Take out our input driver */
  unload_inlib(inlib_main);

  SDL_Quit();
}

void sdl_pixel(int x,int y,hwrcolor c) {
  unsigned long *p;

  if (x<vid->clip_x1 || x>vid->clip_x2 ||
      y<vid->clip_y1 || y>vid->clip_y2)
    return;

  sdl_addarea(x,y,1,1);

  /* SDL doesn't have a good ol' pixel function... */

  switch (vid->bpp) {
  case 8:
    ((unsigned char *)sdl_vidsurf->pixels)[vid->xres*y+x] = c;
    return;
  case 16:
    ((unsigned short *)sdl_vidsurf->pixels)[vid->xres*y+x] = c;
    return;
  case 32:
    ((unsigned long *)sdl_vidsurf->pixels)[vid->xres*y+x] = c;
    return;
  case 24:
    p = ((unsigned char *)sdl_vidsurf->pixels) + (vid->xres*y+x)*3;
    *(((unsigned char *)p)++) = (unsigned char) c;
    *(((unsigned char *)p)++) = (unsigned char) (c>>8);
    *((unsigned char *)p) = (unsigned char) (c>>16);
  }
}

hwrcolor sdl_getpixel(int x,int y) {
  switch (vid->bpp) {
  case 8:
    return ((unsigned char *)sdl_vidsurf->pixels)[vid->xres*y+x];
  case 16:
    return ((unsigned short *)sdl_vidsurf->pixels)[vid->xres*y+x];
  case 32:
    return ((unsigned long *)sdl_vidsurf->pixels)[vid->xres*y+x];
  case 24:
    return *((unsigned long *)(((unsigned char *)sdl_vidsurf->
				pixels) + (vid->xres*y+x)*3));
  }
}

void sdl_update(void) {
  if (!sdl_upd_w) return;

  SDL_UpdateRect(sdl_vidsurf,sdl_upd_x,sdl_upd_y,sdl_upd_w,sdl_upd_h);
  sdl_upd_x = 0;
  sdl_upd_y = 0;
  sdl_upd_w = 0;
  sdl_upd_h = 0;
}

void sdl_blit(struct stdbitmap *src,int src_x,int src_y,
		 struct stdbitmap *dest,int dest_x,int dest_y,
		 int w,int h,int lgop) {
  struct stdbitmap screenb;

  int i,j,s_of,d_of;
  unsigned char *s,*sl,*d,*dl;

  if (lgop==LGOP_NULL) return;

  if (src && (w>(src->w-src_x) || h>(src->h-src_y))) {
    int i,j;

    /* Do a tiled blit */
    for (i=0;i<w;i+=src->w)
      for (j=0;j<h;j+=src->h)
        sdl_blit(src,0,0,dest,dest_x+i,dest_y+j,src->w,src->h,lgop);

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

  screenb.bits = sdl_vidsurf->pixels;
  screenb.w    = vid->xres;
  screenb.h    = vid->yres;

  if (!src) {
    src = &screenb;
  }
  if (!dest) {
    dest = &screenb;
    sdl_addarea(dest_x,dest_y,w,h);
  }

  /* set up pointers */
  s_of = src->w * vid->bpp / 8;
  d_of = dest->w * vid->bpp / 8;
  s = src->bits + src_x*vid->bpp/8 + src_y*s_of;
  d = dest->bits + dest_x*vid->bpp/8 + dest_y*d_of;
  w = w*vid->bpp/8;

  /* Now the actual blitter code depends on the LGOP */
  switch (lgop) {
    
  case LGOP_NONE:
    for (;h;h--,s+=s_of,d+=d_of)
      memcpy(d,s,w);
      break;
    
  case LGOP_OR:
    for (sl=s,dl=d;h;h--,s=(sl+=s_of),d=(dl+=d_of))
      for (i=w;i;i--,d++,s++)
        *d |= *s;
    break;
    
  case LGOP_AND:
    for (sl=s,dl=d;h;h--,s=(sl+=s_of),d=(dl+=d_of))
      for (i=w;i;i--,d++,s++)
        *d &= *s;
    break;
    
  case LGOP_XOR:
    for (sl=s,dl=d;h;h--,s=(sl+=s_of),d=(dl+=d_of))
      for (i=w;i;i--,d++,s++)
        *d ^= *s;
    break;
    
  case LGOP_INVERT:
    for (sl=s,dl=d;h;h--,s=(sl+=s_of),d=(dl+=d_of))
      for (i=w;i;i--,d++,s++)
        *d = (*s) ^ 0xFFFFFF;
    break;
    
  case LGOP_INVERT_OR:
    for (sl=s,dl=d;h;h--,s=(sl+=s_of),d=(dl+=d_of))
      for (i=w;i;i--,d++,s++)
        *d |= (*s) ^ 0xFFFFFF;
    break;
    
  case LGOP_INVERT_AND:
    for (sl=s,dl=d;h;h--,s=(sl+=s_of),d=(dl+=d_of))
      for (i=w;i;i--,d++,s++)
        *d &= (*s) ^ 0xFFFFFF;
    break;
    
  case LGOP_INVERT_XOR:
    for (sl=s,dl=d;h;h--,s=(sl+=s_of),d=(dl+=d_of))
      for (i=w;i;i--,d++,s++)
        *d ^= (*s) ^ 0xFFFFFF;
    break;
  }
}

void sdl_clip_set(int x1,int y1,int x2,int y2) {
  SDL_SetClipping(sdl_vidsurf,y1,x1,y2,x2);
  vid->clip_x1 = x1;
  vid->clip_y1 = y1;
  vid->clip_x2 = x2;
  vid->clip_y2 = y2;
}

void sdl_rect(int x,int y,int w,int h,hwrcolor c) {
  SDL_Rect r;

  if ((x+w-1)>vid->clip_x2) w = vid->clip_x2-x+1;
  if ((y+h-1)>vid->clip_y2) h = vid->clip_y2-y+1;
  if (x<vid->clip_x1) {
    w -= vid->clip_x1 - x;
    x = vid->clip_x1;
  }
  if (y<vid->clip_y1) {
    h -= vid->clip_y1 - y;
    y = vid->clip_y1;
  }
  if (w<=0 || h<=0) return;

  sdl_addarea(x,y,w,h);

  r.x = x;
  r.y = y;
  r.w = w;
  r.h = h;
  SDL_FillRect(sdl_vidsurf,&r,c);
}

hwrcolor sdl_color_pgtohwr(pgcolor c) {
  return SDL_MapRGB(sdl_vidsurf->format,getred(c),getgreen(c),getblue(c));
}

pgcolor sdl_color_hwrtopg(hwrcolor c) {
  unsigned char r,g,b;
  SDL_GetRGB(c,sdl_vidsurf->format,&r,&g,&b);
  return mkcolor(r,g,b);
}


/* Eek!  See the def_gradient for more comments.  This one is just
   reworked for the SDL driver
*/
void sdl_gradient(int x,int y,int w,int h,int angle,
                  pgcolor c1,pgcolor c2,int translucent) {
  /* Lotsa vars! */
  long r_vs,g_vs,b_vs,r_sa,g_sa,b_sa,r_ca,g_ca,b_ca,r_ica,g_ica,b_ica;
  long r_vsc,g_vsc,b_vsc,r_vss,g_vss,b_vss,sc_d;
  long r_v1,g_v1,b_v1,r_v2,g_v2,b_v2;
  unsigned char *p;
  int r,g,b,i,s,c;
  int line_offset;
  hwrcolor hc;

  if ((x+w-1)>vid->clip_x2) w = vid->clip_x2-x+1;
  if ((y+h-1)>vid->clip_y2) h = vid->clip_y2-y+1;
  if (x<vid->clip_x1) {
    w -= vid->clip_x1 - x;
    x = vid->clip_x1;
  }
  if (y<vid->clip_y1) {
    h -= vid->clip_y1 - y;
    y = vid->clip_y1;
  }
  if (w<=0 || h<=0) return;

  sdl_addarea(x,y,w,h);

  /* Look up the sine and cosine */
  angle %= 360;
  if (angle<0) angle += 360;
  if      (angle <= 90 ) s =  trigtab[angle];
  else if (angle <= 180) s =  trigtab[180-angle];
  else if (angle <= 270) s = -trigtab[angle-180];
  else                   s = -trigtab[360-angle];
  angle += 90;
  if (angle>=360) angle -= 360;
  if      (angle <= 90 ) c =  trigtab[angle];
  else if (angle <= 180) c =  trigtab[180-angle];
  else if (angle <= 270) c = -trigtab[angle-180];
  else                   c = -trigtab[360-angle];

  /* Init the bitmap */
  p = ((unsigned char *) sdl_vidsurf->pixels) + 
    x*sdl_vidsurf->format->BytesPerPixel + 
    y*vid->xres*sdl_vidsurf->format->BytesPerPixel;
  line_offset = (vid->xres-w)*sdl_vidsurf->format->BytesPerPixel;

  /* Calculate denominator of the scale value */
  sc_d = h*((s<0) ? -((long)s) : ((long)s)) +
    w*((c<0) ? -((long)c) : ((long)c));

  /* Decode colors */
  r_v1 = getred(c1);
  g_v1 = getgreen(c1);
  b_v1 = getblue(c1);
  r_v2 = getred(c2);
  g_v2 = getgreen(c2);
  b_v2 = getblue(c2);

  /* Calculate the scale values and 
   * scaled sine and cosine (for each channel) */
  r_vs = ((r_v2<<16)-(r_v1<<16)) / sc_d;
  g_vs = ((g_v2<<16)-(g_v1<<16)) / sc_d;
  b_vs = ((b_v2<<16)-(b_v1<<16)) / sc_d;

  /* Zero the accumulators */
  r_sa = g_sa = b_sa = r_ca = g_ca = b_ca = r_ica = g_ica = b_ica = 0;

  /* Calculate the sine and cosine scales */
  r_vsc = (r_vs*((long)c)) >> 8;
  r_vss = (r_vs*((long)s)) >> 8;
  g_vsc = (g_vs*((long)c)) >> 8;
  g_vss = (g_vs*((long)s)) >> 8;
  b_vsc = (b_vs*((long)c)) >> 8;
  b_vss = (b_vs*((long)s)) >> 8;

  /* If the scales are negative, start from the opposite side */
  if (r_vss<0) r_sa  = -r_vss*h;
  if (r_vsc<0) r_ica = -r_vsc*w; 
  if (g_vss<0) g_sa  = -g_vss*h;
  if (g_vsc<0) g_ica = -g_vsc*w; 
  if (b_vss<0) b_sa  = -b_vss*h;
  if (b_vsc<0) b_ica = -b_vsc*w; 

  if (r_v2<r_v1) r_v1 = r_v2;
  if (g_v2<g_v1) g_v1 = g_v2;
  if (b_v2<b_v1) b_v1 = b_v2;

  /* Finally, the loop! */

  if (translucent==0) {
    for (;h;h--,r_sa+=r_vss,g_sa+=g_vss,b_sa+=b_vss,p+=line_offset)
      for (r_ca=r_ica,g_ca=g_ica,b_ca=b_ica,i=w;i;
           i--,r_ca+=r_vsc,g_ca+=g_vsc,b_ca+=b_vsc) {

        r = r_v1 + ((r_ca+r_sa) >> 8);
        g = g_v1 + ((g_ca+g_sa) >> 8);
        b = b_v1 + ((b_ca+b_sa) >> 8);
	hc = SDL_MapRGB(sdl_vidsurf->format,r,g,b);
	
	switch (vid->bpp) {
	case 8:
	  *(p++) = hc;
	  break;
	
	case 16:
	  *(((unsigned short *)p)++) = hc;
	  break;

	case 24:
	  *(p++) = (unsigned char) hc;
	  *(p++) = (unsigned char) (hc >> 8);
	  *(p++) = (unsigned char) (hc >> 16);
	  break;
	  
	case 32:
	  *(((unsigned long *)p)++) = hc;
	}
      }
  }
  else if (translucent>0) {
    for (;h;h--,r_sa+=r_vss,g_sa+=g_vss,b_sa+=b_vss,p+=line_offset)
      for (r_ca=r_ica,g_ca=g_ica,b_ca=b_ica,i=w;i;
           i--,r_ca+=r_vsc,g_ca+=g_vsc,b_ca+=b_vsc) {

	switch (vid->bpp) {
	case 8:
	  hc = *p;
	  break;
	
	case 16:
	  hc = *((unsigned short *)p);
	  break;

	case 24:
	  hc = (p[2]<<16) | (p[1]<<8) | *p;
	  break;
	  
	case 32:
	  hc = *((unsigned long *)p);
	}

	r=g=b=0;
      	SDL_GetRGB(hc,sdl_vidsurf->format,(int*)&r,(int*)&g,(int*)&b);
        r += r_v1 + ((r_ca+r_sa) >> 8);
        g += g_v1 + ((g_ca+g_sa) >> 8);
        b += b_v1 + ((b_ca+b_sa) >> 8);
	if (r>255) r = 255;
	if (g>255) g = 255;
	if (b>255) b = 255;
	hc = SDL_MapRGB(sdl_vidsurf->format,r,g,b);
	
	switch (vid->bpp) {
	case 8:
	  *(p++) = hc;
	  break;
	
	case 16:
	  *(((unsigned short *)p)++) = hc;
	  break;

	case 24:
	  *(p++) = (unsigned char) hc;
	  *(p++) = (unsigned char) (hc >> 8);
	  *(p++) = (unsigned char) (hc >> 16);
	  break;
	  
	case 32:
	  *(((unsigned long *)p)++) = hc;
	}
      }
  }
  else {
    for (;h;h--,r_sa+=r_vss,g_sa+=g_vss,b_sa+=b_vss,p+=line_offset)
      for (r_ca=r_ica,g_ca=g_ica,b_ca=b_ica,i=w;i;
           i--,r_ca+=r_vsc,g_ca+=g_vsc,b_ca+=b_vsc) {

	switch (vid->bpp) {
	case 8:
	  hc = *p;
	  break;
	
	case 16:
	  hc = *((unsigned short *)p);
	  break;

	case 24:
	  hc = (p[2]<<16) | (p[1]<<8) | *p;
	  break;
	  
	case 32:
	  hc = *((unsigned long *)p);
	}

	r=g=b=0;
      	SDL_GetRGB(hc,sdl_vidsurf->format,(int*)&r,(int*)&g,(int*)&b);
        r -= r_v1 + ((r_ca+r_sa) >> 8);
        g -= g_v1 + ((g_ca+g_sa) >> 8);
        b -= b_v1 + ((b_ca+b_sa) >> 8);
	if (r<0) r = 0;
	if (g<0) g = 0;
	if (b<0) b = 0;
	hc = SDL_MapRGB(sdl_vidsurf->format,r,g,b);
	
	switch (vid->bpp) {
	case 8:
	  *(p++) = hc;
	  break;
	
	case 16:
	  *(((unsigned short *)p)++) = hc;
	  break;

	case 24:
	  *(p++) = (unsigned char) hc;
	  *(p++) = (unsigned char) (hc >> 8);
	  *(p++) = (unsigned char) (hc >> 16);
	  break;
	  
	case 32:
	  *(((unsigned long *)p)++) = hc;
	}
      }
  }
}

/******************************************** Driver registration */

g_error sdl_regfunc(struct vidlib *v) {
  v->init = &sdl_init;
  v->close = &sdl_close;   /* Not strictly required (raw framebuffers
				 might not need it) but SDL needs this */
  v->pixel = &sdl_pixel;
  v->getpixel = &sdl_getpixel;
  v->update = &sdl_update; /* Again not required, but good for SDL */
  v->blit = &sdl_blit;
  v->clip_set = &sdl_clip_set; /* If the underlying driver supports it,
				     go for it. Simplifies this driver a lot. */

  v->rect = &sdl_rect;
  v->color_pgtohwr = &sdl_color_pgtohwr;
  v->color_hwrtopg = &sdl_color_hwrtopg;
  v->gradient = &sdl_gradient;

  return sucess;
}

/* The End */
