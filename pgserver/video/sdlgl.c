/* $Id: sdlgl.c,v 1.1 2002/02/26 18:22:47 micahjd Exp $
 *
 * sdlgl.c - OpenGL driver for picogui, using SDL for portability
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

#include <pgserver/common.h>      /* Needed for any pgserver file */

#include <pgserver/video.h>       /* Always needed for a video driver! */
#include <pgserver/render.h>      /* For data types like 'quad' */
#include <pgserver/input.h>       /* For loading our corresponding input lib */
#include <pgserver/configfile.h>  /* For loading our configuration */

#include <SDL/SDL.h>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#if defined(__APPLE__) && defined(__MACH__)
#include <OpenGL/gl.h>	// Header File For The OpenGL32 Library
#include <OpenGL/glu.h>	// Header File For The GLu32 Library
#else
#include <GL/gl.h>	// Header File For The OpenGL32 Library
#include <GL/glu.h>	// Header File For The GLu32 Library
#endif

SDL_Surface *sdlgl_vidsurf;

/* Determine if it's an operation we can handle ourselves */
#define GL_ISNORMAL(dest,lgop) ((!dest) && (lgop==PG_LGOP_NONE))

#ifdef CONFIG_SDLSKIN
extern s16 sdlfb_display_x;
extern s16 sdlfb_display_y;
extern u16 sdlfb_scale;
#endif

/* Macros to convert from picogui to opengl coordinates */
#define GL_X(x) (x)
#define GL_Y(y) (vid->yres - y)

/************************************************** Basic primitives */

void sdlgl_pixel(hwrbitmap dest,s16 x,s16 y,hwrcolor c,s16 lgop) {
  if (!GL_ISNORMAL(dest,lgop)) {
    def_pixel(dest,x,y,c,lgop);
    return;
  }

  glBegin(GL_QUADS);
  glColor3f(getred(c)/255.0f, 
	    getgreen(c)/255.0f,
	    getblue(c)/255.0f);
  glVertex3f(GL_X(x)  ,GL_Y(y)  ,0.0f);
  glVertex3f(GL_X(x+1),GL_Y(y)  ,0.0f);
  glVertex3f(GL_X(x+1),GL_Y(y+1),0.0f);
  glVertex3f(GL_X(x)  ,GL_Y(y+1),0.0f);
  glEnd();
}

/* This is _really_ damn slow, like the X11 getpixel,
 * but with all this fun hardware acceleration we shouldn't
 * actually have to use it much.
 */
hwrcolor sdlgl_getpixel(hwrbitmap dest,s16 x,s16 y) {
  u8 r,g,b;

  if (!GL_ISNORMAL(dest,PG_LGOP_NONE))
    return def_getpixel(dest,x,y);
 
  glReadPixels(GL_X(x),GL_Y(y),1,1,GL_RED,GL_UNSIGNED_BYTE,&r);
  glReadPixels(GL_X(x),GL_Y(y),1,1,GL_GREEN,GL_UNSIGNED_BYTE,&g);
  glReadPixels(GL_X(x),GL_Y(y),1,1,GL_BLUE,GL_UNSIGNED_BYTE,&b);
  return mkcolor(r,g,b);
}

/* It's double-buffered */
void sdlgl_update(s16 x, s16 y, s16 w, s16 h) {
  SDL_GL_SwapBuffers();
};  

void sdlgl_rect(hwrbitmap dest,s16 x,s16 y,s16 w, s16 h, hwrcolor c,s16 lgop) {
  if (!GL_ISNORMAL(dest,lgop)) {
    def_rect(dest,x,y,w,h,c,lgop);
    return;
  }

  glBegin(GL_QUADS);
  glColor3f(getred(c)/255.0f, 
	    getgreen(c)/255.0f,
	    getblue(c)/255.0f);
  glVertex3f(GL_X(x)  ,GL_Y(y)  ,0.0f);
  glVertex3f(GL_X(x+w),GL_Y(y)  ,0.0f);
  glVertex3f(GL_X(x+w),GL_Y(y+h),0.0f);
  glVertex3f(GL_X(x)  ,GL_Y(y+h),0.0f);
  glEnd();
}


/************************************************** Initialization */

g_error sdlgl_init(void) {
  /* Default mode: 640x480 */
  if (!vid->xres) vid->xres = 640;
  if (!vid->yres) vid->yres = 480;

  /* Start up the SDL video subsystem thingy */
  if (SDL_Init(SDL_INIT_VIDEO))
    return mkerror(PG_ERRT_IO,46);

  /* We're _not_ using a normal framebuffer */
  vid->display = NULL;

#ifdef CONFIG_SDLSKIN
  /* If we've got SDL skinning support, set the relevant vars to good values 
   * so the input driver won't get wacky on us.
   */
  sdlfb_display_x = 0;
  sdlfb_display_y = 0;
  sdlfb_scale     = 1;
#endif

  /* Load a main input driver */
  return load_inlib(&sdlinput_regfunc,&inlib_main);
}

g_error sdlgl_setmode(s16 xres,s16 yres,s16 bpp,u32 flags) {
  unsigned long sdlflags = SDL_RESIZABLE | SDL_OPENGL;
  char str[80];
   
  /* Interpret flags */
  if (get_param_int("video-sdlgl","fullscreen",0))
    sdlflags |= SDL_FULLSCREEN;

  /* Set the video mode */
  if ((!sdlgl_vidsurf) || xres != vid->xres || yres !=vid->yres) {
     sdlgl_vidsurf = SDL_SetVideoMode(xres,yres,0,sdlflags);
     if (!sdlgl_vidsurf)
       return mkerror(PG_ERRT_IO,47);
  }

  /* Always use true color */
  vid->bpp = 32;
  vid->xres = xres;
  vid->yres = yres;
  
  /* Info */
  snprintf(str,sizeof(str),get_param_str("video-sdlgl","caption","PicoGUI (sdlgl@%dx%d)"),
	   vid->xres,vid->yres,bpp);
  SDL_WM_SetCaption(str,NULL);

  /* OpenGL setup */
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glLoadIdentity();
  glViewport(0, 0, xres, yres);
  glOrtho(0, xres, 0, yres, -1, 1);
 
  return success; 
}
   
void sdlgl_close(void) {
  unload_inlib(inlib_main);   /* Take out our input driver */
  SDL_Quit();
}

/************************************************** Registration */

g_error sdlgl_regfunc(struct vidlib *v) {
  setvbl_default(v);

  v->init = &sdlgl_init;
  v->setmode = &sdlgl_setmode; 
  v->close = &sdlgl_close;
  v->pixel = &sdlgl_pixel;
  v->getpixel = &sdlgl_getpixel;
  v->update = &sdlgl_update;
  return success;
}

/* The End */









