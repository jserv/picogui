/* $Id$
 *
 * sdlgl.c - Video driver using SDL and the OpenGL VBL 
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
#include <pgserver/input.h>
#include <pgserver/configfile.h>
#include <pgserver/gl.h>
#include <SDL/SDL.h> 

#ifdef CONFIG_SDLSKIN
extern s16 sdlfb_display_x;
extern s16 sdlfb_display_y;
extern u16 sdlfb_scale;
#endif

g_error sdlgl_init(void) {
  g_error e;

  /* Default mode: 640x480 */
  if (!vid->xres) vid->xres = 640;
  if (!vid->yres) vid->yres = 480;

  /* Start up the SDL video subsystem thingy */
  if (SDL_Init(SDL_INIT_VIDEO))
    return mkerror(PG_ERRT_IO,46);

  /* VBL init */
  e = gl_init();
  errorcheck;

  /* Load a main input driver */
  return load_inlib(&sdlinput_regfunc,&inlib_main);
}

g_error sdlgl_setmode(s16 xres,s16 yres,s16 bpp,u32 flags) {
  u32 sdlflags = SDL_RESIZABLE | SDL_OPENGL;
  char str[80];
  g_error e;
  static SDL_Surface *surface = NULL;

  /* Interpret flags */
  if (get_param_int("video-sdlgl","fullscreen",0))
    sdlflags |= SDL_FULLSCREEN;
   
  /* Set the video mode */
  if ((!surface) || xres != vid->xres || 
      yres !=vid->yres || bpp != vid->bpp) {
    surface = SDL_SetVideoMode(xres,yres,bpp,sdlflags);
    if (!surface)
      return mkerror(PG_ERRT_IO,47);
  }

  vid->bpp  = surface->format->BitsPerPixel;
  vid->xres = xres;
  vid->yres = yres;

#ifdef CONFIG_SDLSKIN
  /* If we've got SDL skinning support, set the relevant vars to good values 
   * so the input driver won't get wacky on us.
   */
  sdlfb_display_x = 0;
  sdlfb_display_y = 0;
  sdlfb_scale     = 1;
#endif
   
  e = gl_setmode(xres,yres,bpp,flags);
  errorcheck;

  snprintf(str,sizeof(str),get_param_str("video-sdlgl","caption","PicoGUI (sdlgl@%dx%dx%d)"),
	   vid->xres,vid->yres,bpp);
  SDL_WM_SetCaption(str,NULL);
  
  return success; 
}
   
void sdlgl_close(void) {
  gl_close();
  unload_inlib(inlib_main);   /* Take out our input driver */
  inlib_main = NULL;
  SDL_Quit();
}

void sdlgl_update(hwrbitmap d,s16 x,s16 y,s16 w,s16 h) {
  SDL_GL_SwapBuffers();
}

g_error sdlgl_regfunc(struct vidlib *v) {
  setvbl_gl(v);
  v->init = &sdlgl_init;
  v->setmode = &sdlgl_setmode; 
  v->close = &sdlgl_close;
  v->update = &sdlgl_update;    
  return success;
}

/* The End */









