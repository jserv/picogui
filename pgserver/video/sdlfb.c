/* $Id: sdlfb.c,v 1.9 2001/03/01 02:23:11 micahjd Exp $
 *
 * sdlfb.c - Video driver for SDL using a linear framebuffer.
 *           This will soon replace sdl.c, but only after the
 *           linear* VBLs are well tested. Right now it's a testbed
 *           for those VBLs.
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
#include <pgserver/input.h>

#include <SDL.h>

SDL_Surface *sdl_vidsurf;
#if defined(CONFIG_SDLEMU_COLOR) || defined(CONFIG_SDLEMU_BLIT)
int sdlfb_emucolors;
#endif

hwrcolor sdlfbemu_color_pgtohwr(pgcolor c);
pgcolor sdlfbemu_color_hwrtopg(hwrcolor c);
g_error sdlfb_init(int xres,int yres,int bpp,unsigned long flags);
void sdlfb_close(void);
void sdlfb_update(int x,int y,int w,int h);
g_error sdlfb_regfunc(struct vidlib *v);

g_error sdlfb_init(int xres,int yres,int bpp,unsigned long flags) {
  unsigned long sdlflags = 0;
  char str[80];
  SDL_Color palette[256];
  int i;

  /* Avoid freeing a nonexistant backbuffer in close() */
  vid->fb_mem = NULL;
   
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
  sdl_vidsurf = SDL_SetVideoMode(xres,yres,(bpp && bpp<8) ? 8 : bpp,sdlflags);
  if (!sdl_vidsurf)
    return mkerror(PG_ERRT_IO,47);

  /* Use the default depth? */
  if (!bpp)
     bpp  = sdl_vidsurf->format->BitsPerPixel;
   
  /* If we're emulating low bpp with color conversion, load custom
   * color functions and a palette */
#if defined(CONFIG_SDLEMU_COLOR) || defined(CONFIG_SDLEMU_BLIT)
  if (bpp<8) {
     int colors = 1<<bpp;
     pgcolor pc;
     sdlfb_emucolors = colors-1;
     for (i=0;i<colors;i++) {
	pc = sdlfbemu_color_hwrtopg(i);
	palette[i].r = getred(pc);
	palette[i].g = getgreen(pc);
	palette[i].b = getblue(pc);
     }
     SDL_SetColors(sdl_vidsurf,palette,0,colors);
#ifdef CONFIG_SDLEMU_COLOR
     bpp = 8;
#endif
  }
   else
     sdlfb_emucolors = 0;
#endif /* defined(CONFIG_SDLEMU_COLOR) || defined(CONFIG_SDLEMU_BLIT) */
   
  /* Load a VBL */
  switch (bpp) {
     
#ifdef CONFIG_SDLEMU_BLIT
     /* Low bit depths */
     
#ifdef CONFIG_VBL_LINEAR1
   case 1:
     setvbl_linear1(vid);
     break;
#endif

#ifdef CONFIG_VBL_LINEAR2
   case 2:
     setvbl_linear2(vid);
     break;
#endif

#ifdef CONFIG_VBL_LINEAR4
   case 4:
     setvbl_linear4(vid);
     break;
#endif

#endif /* CONFIG_SDLEMU_BLIT */
     
#ifdef CONFIG_VBL_LINEAR8
   case 8:
     setvbl_linear8(vid);

#ifdef CONFIG_SDLEMU_COLOR
     if (sdlfb_emucolors) {
	vid->color_pgtohwr = &sdlfbemu_color_pgtohwr;
	vid->color_hwrtopg = &sdlfbemu_color_hwrtopg;
     }
     else
     /* If this is 8bpp set up a 2-3-3 palette for pseudo-RGB */
#endif
     {
	for (i=0;i<256;i++) {
	   palette[i].r = (i & 0xC0) * 255 / 0xC0;
	   palette[i].g = (i & 0x38) * 255 / 0x38;
	   palette[i].b = (i & 0x07) * 255 / 0x07;
	}
	SDL_SetColors(sdl_vidsurf,palette,0,256);
     }
     break;
#endif
     
#ifdef CONFIG_VBL_LINEAR16
   case 16:
     setvbl_linear16(vid);
     break;
#endif

   default:
      sdlfb_close();
      return mkerror(PG_ERRT_BADPARAM,101);   /* Unknown bpp */
  }
   
  /* Save the actual video mode (might be different than what
     was requested) */
  vid->xres = sdl_vidsurf->w;
  vid->yres = sdl_vidsurf->h;
#ifdef CONFIG_SDLEMU_BLIT
  /* If we're blitting to a higher bpp, use another backbuffer */
  if (bpp<8) {
     g_error e;
     
     vid->bpp = bpp;
     vid->fb_bpl = (vid->xres * bpp) >> 3;
     e = g_malloc((void**)&vid->fb_mem,vid->fb_bpl * vid->yres);
     errorcheck;
  }
  else
#endif
  {
     vid->bpp  = sdl_vidsurf->format->BitsPerPixel;
     vid->fb_mem = sdl_vidsurf->pixels;
     vid->fb_bpl = sdl_vidsurf->pitch;
  }
   
  /* Info */
  sprintf(str,"PicoGUI (sdl@%dx%dx%d)",
	  vid->xres,vid->yres,bpp);
  SDL_WM_SetCaption(str,NULL);

  /* Load a main input driver */
  return load_inlib(&sdlinput_regfunc,&inlib_main);
}

void sdlfb_close(void) {
#ifdef CONFIG_SDLEMU_BLIT
  /* Free backbuffer */
   if (vid->fb_mem && (vid->fb_mem != sdl_vidsurf->pixels))
     g_free(vid->fb_mem);
#endif   
  unload_inlib(inlib_main);   /* Take out our input driver */
  SDL_Quit();
}

void sdlfb_update(int x,int y,int w,int h) {
#ifdef DEBUG_VIDEO
   printf("sdlfb_update(%d,%d,%d,%d)\n",x,y,w,h);
#endif

#ifdef CONFIG_SDLEMU_BLIT
   /* Do we need to convert and blit to the SDL buffer? */
   if (vid->fb_mem != sdl_vidsurf->pixels) {
      unsigned char *src;
      unsigned char *dest;
      unsigned char *srcline;
      unsigned char *destline;
      int i,bw,j;
      int maxshift = 8 - vid->bpp;
      int shift;
      unsigned char c, mask = (1<<vid->bpp) - 1;

      /* Align it to an 8-pixel boundary (simplifies blit) */
      w += (x&7) + 7;
      w &= ~7;
      x &= ~7;
      
      /* Calculations */
      srcline = src = vid->fb_mem + ((x * vid->bpp) >> 3) +y*vid->fb_bpl;
      destline = dest = sdl_vidsurf->pixels + x + y*vid->xres;
      bw = (w * vid->bpp) >> 3;
      
      /* Slow but it works (this is debug code, after all...) */
      for (j=h;j;j--,src=srcline+=vid->fb_bpl,dest=destline+=sdl_vidsurf->pitch)
	for (i=bw;i;i--,src++)
	  for (shift=maxshift,c=*src;shift>=0;shift-=vid->bpp)
	    *(dest++) = (c >> shift) & mask;
   }
#endif

   /* Always let SDL update the front buffer */
   SDL_UpdateRect(sdl_vidsurf,x,y,w,h);
}

#if defined(CONFIG_SDLEMU_COLOR) || defined(CONFIG_SDLEMU_BLIT)
hwrcolor sdlfbemu_color_pgtohwr(pgcolor c) {
   /* If this is black and white, be more conservative */
   if (sdlfb_emucolors==1)
     return (getred(c)+getgreen(c)+getblue(c))>0x80;
   else
     return (getred(c)+getgreen(c)+getblue(c))*sdlfb_emucolors/765;   
}

pgcolor sdlfbemu_color_hwrtopg(hwrcolor c) {
   unsigned char gray = c * 255/sdlfb_emucolors;
   return mkcolor(gray,gray,gray);
}
#endif

g_error sdlfb_regfunc(struct vidlib *v) {
  v->init = &sdlfb_init;
  v->close = &sdlfb_close;
  v->update = &sdlfb_update;    
  return sucess;
}

/* The End */
