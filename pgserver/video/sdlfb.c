/* $Id: sdlfb.c,v 1.4 2001/01/10 13:10:42 micahjd Exp $
 *
 * sdlfb.c - Video driver for SDL using a linear framebuffer.
 *           This will soon replace sdl.c, but only after the
 *           linear* VBLs are well tested. Right now it's a testbed
 *           for those VBLs.
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

#ifdef DRIVER_SDLFB

#include <pgserver/video.h>
#include <pgserver/input.h>

#include <SDL.h>

SDL_Surface *sdl_vidsurf;

g_error sdlfb_init(int xres,int yres,int bpp,unsigned long flags) {
  unsigned long sdlflags = 0;
  char str[80];

  /* Default mode: 640x480 */
  if (!xres) xres = 640;
  if (!yres) yres = 480;

  /* Only linear8 is done so far, force to 8bpp */
  bpp = 8;

  /* Start up the SDL video subsystem thingy */
  if (SDL_Init(SDL_INIT_VIDEO))
    return mkerror(PG_ERRT_IO,46);

  /* Interpret flags */
  if (flags & PG_VID_FULLSCREEN)
    sdlflags |= SDL_FULLSCREEN;

  /* Set the video mode */
  sdl_vidsurf = SDL_SetVideoMode(xres,yres,bpp,sdlflags);
  if (!sdl_vidsurf)
    return mkerror(PG_ERRT_IO,47);

  /* Save the actual video mode (might be different than what
     was requested) */
  vid->xres = sdl_vidsurf->w;
  vid->yres = sdl_vidsurf->h;
  vid->bpp  = sdl_vidsurf->format->BitsPerPixel;

  /* Save the linear framebuffer */
  vid->fb_mem = sdl_vidsurf->pixels;
  vid->fb_bpl = sdl_vidsurf->pitch;

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

  /* Load a main input driver */
  return load_inlib(&sdlinput_regfunc,&inlib_main);
}

void sdlfb_close(void) {
  unload_inlib(inlib_main);   /* Take out our input driver */
  SDL_Quit();
}

void sdlfb_update(int x,int y,int w,int h) {
  SDL_UpdateRect(sdl_vidsurf,x,y,w,h);
}

g_error sdlfb_regfunc(struct vidlib *v) {
  setvbl_linear8(v);          /* For now just support 8bpp */
  v->init = &sdlfb_init;
  v->close = &sdlfb_close;
  v->update = &sdlfb_update;    
  return sucess;
}

#endif /* DRIVER_SDLFB */
/* The End */
