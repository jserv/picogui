/* $Id: sdlgl_init.c,v 1.20 2002/11/08 01:25:37 micahjd Exp $
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

  /* Load an inlib to handle rendering continuously (or not) */
  e = load_inlib(&gl_continuous_regfunc,&gl_global.continuous_inlib);
  errorcheck;

  s = get_param_str(GL_SECTION,"texture_filtering","linear");
  if (!strcmp(s,"linear")) {
    gl_global.texture_filtering = GL_LINEAR;
  }
  else {
    gl_global.texture_filtering = GL_NEAREST;
  }

  sscanf(get_param_str(GL_SECTION,"fps_interval","0.25"),"%f",&gl_global.fps_interval);

  /* Load our input filter */
  e = infilter_insert(&infilter_list, &gl_global.h_infilter, -1, &infilter_sdlgl);
  errorcheck;
  
  /* Load a main input driver */
  return load_inlib(&sdlinput_regfunc,&inlib_main);
}

g_error sdlgl_setmode(s16 xres,s16 yres,s16 bpp,u32 flags) {
  u32 sdlflags = SDL_RESIZABLE | SDL_OPENGL;
  char str[80];
  float a,x,y,z;
  g_error e;
  int zoom = get_param_int(GL_SECTION,"zoom",1);
   
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
  vid->xres = xres/zoom;
  vid->yres = yres/zoom;

  /* Info */
  snprintf(str,sizeof(str),get_param_str(GL_SECTION,"caption","PicoGUI (sdlgl@%dx%d)"),
	   vid->xres,vid->yres,bpp);
  SDL_WM_SetCaption(str,NULL);

  gl_global.continuous = get_param_int(GL_SECTION,"continuous",0);

  /********** OpenGL setup */

  /* Clear */
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glClearColor(0.0f, 0.4f, 0.0f, 0.0f);
  glClearDepth(1.0);

  gl_global.antialias = get_param_int(GL_SECTION,"antialias",0);
  if (gl_global.antialias) {
    glEnable(GL_POLYGON_SMOOTH);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_BLEND);
  }

  /* Set up camera */
  glViewport(0,0,xres,yres);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(GL_FOV,1,GL_MINDEPTH,GL_MAXDEPTH*2);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  gl_matrix_pixelcoord();

  return success; 
}
   
void sdlgl_close(void) {

  if (gl_global.h_infilter)
    handle_free(-1,gl_global.h_infilter);

  if (gl_global.display_rend) {
    g_free(gl_global.display_rend);
    gl_global.display_rend = NULL;
  }

  unload_inlib(inlib_main);   /* Take out our input driver */
  unload_inlib(gl_global.continuous_inlib);

  if (gl_global.osd_font)
    font_descriptor_destroy(gl_global.osd_font);

  SDL_Quit();
}

void gl_continuous_init(int *n,fd_set *readfds,struct timeval *timeout) {
  timeout->tv_sec = 0;
  if (gl_global.continuous)
    timeout->tv_usec = 0;
  else
    timeout->tv_usec = 1;
}

void gl_continuous_poll(void) {
  if (gl_global.continuous || gl_global.need_update || gl_global.camera_mode)
    gl_frame();
}

g_error gl_continuous_regfunc(struct inlib *i) {
  i->poll = gl_continuous_poll;
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
  v->update_hook = &sdlgl_update_hook;
  v->sprite_show = &sdlgl_sprite_show;
  v->sprite_hide = &sdlgl_sprite_hide;
  v->sprite_update = &sdlgl_sprite_update;
  v->sprite_protectarea = &sdlgl_sprite_protectarea;
  v->grop_render_presetup_hook = &sdlgl_grop_render_presetup_hook;
  v->color_pgtohwr = &sdlgl_color_pgtohwr;
  v->color_hwrtopg = &sdlgl_color_hwrtopg;
  v->grop_handler = &sdlgl_grop_handler;
  v->grop_render_node_hook = &sdlgl_grop_render_node_hook;
  v->grop_render_postsetup_hook = &sdlgl_grop_render_postsetup_hook;
  v->grop_render_end_hook = &sdlgl_grop_render_end_hook;
  v->bitmap_getshm = &sdlgl_bitmap_getshm;
  //  v->charblit = &sdlgl_charblit;
  v->multiblit = &sdlgl_multiblit;
#ifdef CONFIG_FONTENGINE_FREETYPE
  //  v->alpha_charblit = &sdlgl_alpha_charblit;
#endif

  return success;
}

/* The End */









