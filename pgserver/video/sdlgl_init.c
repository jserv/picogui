/* $Id: sdlgl_init.c,v 1.4 2002/03/03 11:21:11 micahjd Exp $
 *
 * sdlgl_init.c - OpenGL driver for picogui, using SDL for portability.
 *                This file has initialization, shutdown, and registration.
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
#include <pgserver/sdlgl.h>

g_error sdlgl_init(void) {
  const char *s;
  int i;
  g_error e;

  /* Default mode: 640x480 */
  if (!vid->xres) vid->xres = 640;
  if (!vid->yres) vid->yres = 480;

  /* Start up the SDL video subsystem thingy and the SDL_ttf library */
  if (SDL_Init(SDL_INIT_VIDEO) || TTF_Init())
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

  /* Optionally load the continuous-running "input driver" */
  if (get_param_int(GL_SECTION,"continuous",0)) {
    e = load_inlib(&gl_continuous_regfunc,&gl_global.continuous);
    errorcheck;
  }

  s = get_param_str(GL_SECTION,"texture_filtering","linear");
  if (!strcmp(s,"linear")) {
    gl_global.texture_filtering = GL_LINEAR;
  }
  else {
    gl_global.texture_filtering = GL_NEAREST;
  }

  sscanf(get_param_str(GL_SECTION,"fps_interval","0.25"),"%f",&gl_global.fps_interval);
  
  /* Load a main input driver */
  return load_inlib(&sdlinput_regfunc,&inlib_main);
}

g_error sdlgl_setmode(s16 xres,s16 yres,s16 bpp,u32 flags) {
  unsigned long sdlflags = SDL_RESIZABLE | SDL_OPENGL;
  char str[80];
  float a,x,y,z;
  g_error e;
   
  /* Interpret flags */
  if (get_param_int(GL_SECTION,"fullscreen",0))
    sdlflags |= SDL_FULLSCREEN;

  /* Set the video mode */
  if ((!gl_global.vidsurf) || xres != vid->xres || yres !=vid->yres) {
     gl_global.vidsurf = SDL_SetVideoMode(xres,yres,0,sdlflags);
     if (!gl_global.vidsurf)
       return mkerror(PG_ERRT_IO,47);
  }

  /* Always use true color */
  vid->bpp = 32;
  vid->xres = xres;
  vid->yres = yres;
  
  /* Info */
  snprintf(str,sizeof(str),get_param_str(GL_SECTION,"caption","PicoGUI (sdlgl@%dx%d)"),
	   vid->xres,vid->yres,bpp);
  SDL_WM_SetCaption(str,NULL);

  /********** OpenGL setup */

  /* Clear */
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  glClearDepth(1.0);

  //  glDepthFunc(GL_LEQUAL);
  //  glEnable(GL_DEPTH_TEST);

  gl_global.antialias = get_param_int(GL_SECTION,"antialias",0);
  if (gl_global.antialias) {
    glEnable(GL_POLYGON_SMOOTH);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_BLEND);
  }

  /* Set up camera */
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(GL_FOV,1,GL_MINDEPTH,GL_MAXDEPTH);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  gl_matrix_pixelcoord();

  /* Set up fonts, if necessary */
  if (!get_param_int(GL_SECTION,"standard_fonts",0)) {
    struct gl_fontload *fl;

    /* Remove normal picogui fonts */
    gl_global.old_fonts = fontstyles;
    fontstyles = NULL;

    e = gl_fontload_init(&fl);
    errorcheck;

    e = gl_load_font(fl,"/usr/share/fonts/truetype/openoffice/helmetb.ttf");  
    errorcheck;

    gl_fontload_finish(fl);
  }

  e = findfont(&gl_global.osd_font,-1,NULL,get_param_int(GL_SECTION,"osd_fontsize",20),0);
  errorcheck;

  return success; 
}
   
void sdlgl_close(void) {
  struct fontstyle_node *f;

  if (gl_global.display_rend) {
    g_free(gl_global.display_rend);
    gl_global.display_rend = NULL;
  }

  unload_inlib(inlib_main);   /* Take out our input driver */
  if (gl_global.continuous)
    unload_inlib(gl_global.continuous);

  if (gl_global.osd_font)
    handle_free(gl_global.osd_font,-1);

  if (gl_global.old_fonts) {
    /* Delete our fonts */
    while (fontstyles) {
      f = fontstyles;
      fontstyles = f->next;
      gl_fontstyle_free(f);
    }

    /* Put back normal fonts */
    fontstyles = gl_global.old_fonts;
  }

  TTF_Quit();
  SDL_Quit();
}

void gl_continuous_init(int *n,fd_set *readfds,struct timeval *timeout) {
  timeout->tv_sec = 0;
  timeout->tv_usec = 0;
}

g_error gl_continuous_regfunc(struct inlib *i) {
  i->poll = gl_frame;
  i->fd_init = gl_continuous_init;
  return success;
}

g_error sdlgl_regfunc(struct vidlib *v) {
  setvbl_default(v);

  v->init = &sdlgl_init;
  v->setmode = &sdlgl_setmode; 
  v->close = &sdlgl_close;
  v->pixel = &sdlgl_pixel;
  v->getpixel = &sdlgl_getpixel;
  v->update = &sdlgl_update;
  v->slab = &sdlgl_slab;
  v->bar = &sdlgl_bar;
  v->line = &sdlgl_line;
  v->rect = &sdlgl_rect;
  v->gradient = &sdlgl_gradient;
  v->bitmap_get_groprender = &sdlgl_bitmap_get_groprender;
  v->bitmap_getsize = &sdlgl_bitmap_getsize;
  v->bitmap_new = &sdlgl_bitmap_new;
  v->bitmap_free = &sdlgl_bitmap_free;
  v->blit = &sdlgl_blit;
  v->key_event_hook = &sdlgl_key_event_hook;
  v->pointing_event_hook = &sdlgl_pointing_event_hook;

  if (!get_param_int(GL_SECTION,"standard_fonts",0)) {
    v->font_sizetext_hook = &sdlgl_font_sizetext_hook;
    v->font_newdesc = &sdlgl_font_newdesc;
    v->font_outtext_hook = &sdlgl_font_outtext_hook;
  }

  return success;
}

/* The End */









