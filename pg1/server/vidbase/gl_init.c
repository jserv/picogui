/* $Id$
 *
 * sdlgl_init.c - OpenGL driver for picogui, using SDL for portability.
 *                This file has initialization, shutdown, and registration.
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
#include <pgserver/gl.h>


g_error gl_init(void) {
  const char *s;
  g_error e;

  /* Load an inlib to handle rendering continuously (or not) */
  e = load_inlib(&gl_continuous_regfunc,&gl_global.continuous_inlib);
  errorcheck;

  s = get_param_str(GL_SECTION,"texture_filtering","linear");
  if (!strcmp(s,"linear")) {
    gl_global.texture_filtering = GL_LINEAR_MIPMAP_LINEAR;
  }
  else {
    gl_global.texture_filtering = GL_NEAREST;
  }

  sscanf(get_param_str(GL_SECTION,"fps_interval","0.25"),"%f",&gl_global.fps_interval);
  gl_global.continuous = get_param_int(GL_SECTION,"continuous",0);
  gl_global.antialias = get_param_int(GL_SECTION,"antialias",0);

  if (get_param_int(GL_SECTION,"keys",1)) {
    e = infilter_insert(&infilter_list, &gl_global.h_infilter, -1, &infilter_gl);
    errorcheck;
  }
  
  return success;
}

g_error gl_setmode(s16 xres,s16 yres,s16 bpp,u32 flags) {
  glViewport(0,0,xres,yres);
  glClearDepth(1.0);
  
  /* There's no framebuffer equivalent for our display */
  vid->display = NULL;

  return success; 
}
   
void gl_close(void) {
  if (gl_global.h_infilter)
    handle_free(-1,gl_global.h_infilter);

  if (gl_global.display_rend) {
    g_free(gl_global.display_rend);
    gl_global.display_rend = NULL;
  }

  unload_inlib(gl_global.continuous_inlib);

  if (gl_global.osd_font)
    font_descriptor_destroy(gl_global.osd_font);
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

void setvbl_gl(struct vidlib *v) {
  setvbl_default(v);

  v->init = &gl_init;
  v->setmode = &gl_setmode; 
  v->close = &gl_close;
  v->pixel = &gl_pixel;
  v->getpixel = &gl_getpixel;
  v->slab = &gl_slab;
  v->bar = &gl_bar;
  v->line = &gl_line;
  v->rect = &gl_rect;
  v->gradient = &gl_gradient;
  v->bitmap_get_groprender = &gl_bitmap_get_groprender;
  v->bitmap_getsize = &gl_bitmap_getsize;
  v->bitmap_new = &gl_bitmap_new;
  v->bitmap_free = &gl_bitmap_free;
  v->blit = &gl_blit;
  v->update_hook = &gl_update_hook;
  v->sprite_show = &gl_sprite_show;
  v->sprite_hide = &gl_sprite_hide;
  v->sprite_update = &gl_sprite_update;
  v->sprite_protectarea = &gl_sprite_protectarea;
  v->grop_render_presetup_hook = &gl_grop_render_presetup_hook;
  v->color_pgtohwr = &gl_color_pgtohwr;
  v->color_hwrtopg = &gl_color_hwrtopg;
  v->grop_handler = &gl_grop_handler;
  v->grop_render_node_hook = &gl_grop_render_node_hook;
  v->grop_render_postsetup_hook = &gl_grop_render_postsetup_hook;
  v->grop_render_end_hook = &gl_grop_render_end_hook;
  v->bitmap_getshm = &gl_bitmap_getshm;
  v->multiblit = &gl_multiblit;
  v->blur = &gl_blur;
}

/* The End */









