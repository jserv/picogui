/* $Id$
 *
 * dvbl_init.c - This file is part of the Default Video Base Library,
 *               providing the basic video functionality in picogui but
 *               easily overridden by drivers.
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2003 Micah Dowty <micahjd@users.sourceforge.net>
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
 */

#include <pgserver/common.h>

#ifdef DEBUG_VIDEO
#define DEBUG_FILE
#endif
#include <pgserver/debug.h>

#include <pgserver/video.h>
#include <pgserver/font.h>
#include <pgserver/render.h>

/******* trivial functions */

g_error def_setmode(s16 xres,s16 yres,s16 bpp,u32 flags) {
  return success;
}

void emulate_dos(void) {
}

g_error def_enterexitmode(void) {
   return success;
}

void def_update(hwrbitmap dest,s16 x,s16 y,s16 w,s16 h) {
}

void def_coord_logicalize(int *x,int *y) {
}

void def_coord_keyrotate(int *k) {
}

int def_grop_render_presetup_hook(struct divnode **div, struct gropnode ***listp,
				  struct groprender *rend) {
  return 0;
}

void def_grop_render_end_hook(struct divnode **div, struct gropnode ***listp,
			      struct groprender *rend) {
}

int def_grop_render_node_hook(struct divnode **div, struct gropnode ***listp,
			      struct groprender *rend, struct gropnode *node) {
  return 0;
}

int def_update_hook(void) {
  return 0;
}

void def_grop_handler(struct groprender *r, struct gropnode *n) {
}
 
hwrbitmap def_window_debug(void) {
  return vid->display;
}

hwrbitmap def_window_fullscreen(void) {
  return vid->display;
}

g_error def_window_new(hwrbitmap *hbmp, struct divtree *dt) {
  *hbmp = vid->display;
  return success;
}

void def_window_free(hwrbitmap window) {
}

void def_window_set_title(hwrbitmap window, const struct pgstring *title) {
}

void def_window_set_flags(hwrbitmap window, int flags) {
}

void def_window_set_position(hwrbitmap window, s16 x, s16 y) {
}

void def_window_set_size(hwrbitmap window, s16 w, s16 h) {
}

void def_window_get_position(hwrbitmap window, s16 *x, s16 *y) {
  *x = *y = 0;
}

void def_window_get_size(hwrbitmap window, s16 *w, s16 *h) {
  *w = vid->lxres;
  *h = vid->lyres;
}

int def_is_rootless(void) {
  return 0;
}

/******* registration */

void setvbl_default(struct vidlib *vid) {
  /* Set defaults */
  vid->color_pgtohwr = &def_color_pgtohwr;
  vid->color_hwrtopg = &def_color_hwrtopg;
  vid->slab = &def_slab;
  vid->bar = &def_bar;
  vid->line = &def_line;
  vid->rect = &def_rect;
  vid->gradient = &def_gradient;
  vid->multiblit = &def_multiblit;
  vid->scrollblit = &def_scrollblit;
  vid->arc = &def_arc; 
  vid->ellipse = &def_ellipse; 
  vid->fellipse = &def_fellipse; 
  vid->fpolygon = &def_fpolygon;
#ifdef CONFIG_FORMAT_XBM
  vid->bitmap_loadxbm = &def_bitmap_loadxbm;
#endif
  vid->bitmap_load = &def_bitmap_load;
  vid->bitmap_new = &def_bitmap_new;
  vid->bitmap_free = &def_bitmap_free;
  vid->bitmap_getsize = &def_bitmap_getsize;
  vid->sprite_show = &def_sprite_show;
  vid->sprite_hide = &def_sprite_hide;
  vid->sprite_update = &def_sprite_update;
  vid->sprite_showall = &def_sprite_showall;
  vid->sprite_hideall = &def_sprite_hideall;
  vid->sprite_protectarea = &def_sprite_protectarea;
  vid->blit = &def_blit;
  vid->coord_logicalize = &def_coord_logicalize;
  vid->coord_physicalize = &def_coord_logicalize;
  vid->rotateblit = &def_rotateblit;
  vid->entermode = &def_enterexitmode;
  vid->exitmode = &def_enterexitmode;
  vid->bitmap_modeconvert = &def_bitmap_modeconvert;
  vid->bitmap_modeunconvert = &def_bitmap_modeunconvert;
  vid->bitmap_get_groprender = &def_bitmap_get_groprender;
  vid->bitmap_getshm = &def_bitmap_getshm;
  vid->coord_keyrotate = &def_coord_keyrotate;
  vid->grop_render_presetup_hook = &def_grop_render_presetup_hook;
  vid->grop_render_postsetup_hook = &def_grop_render_presetup_hook;
  vid->grop_render_end_hook = &def_grop_render_end_hook; 
  vid->grop_render_node_hook = &def_grop_render_node_hook;
  vid->update_hook = &def_update_hook;
  vid->grop_handler = &def_grop_handler;
  vid->blur = &def_blur;
  vid->charblit = &def_charblit;
  vid->window_debug = &def_window_debug;
  vid->window_fullscreen = &def_window_fullscreen;
  vid->window_new = &def_window_new;
  vid->window_free = &def_window_free;
  vid->window_set_title = &def_window_set_title;
  vid->window_set_position = &def_window_set_position;
  vid->window_get_position = &def_window_get_position;
  vid->window_set_size = &def_window_set_size;
  vid->window_get_size = &def_window_get_size;
  vid->window_set_flags = &def_window_set_flags;
  vid->is_rootless = &def_is_rootless;
#ifdef CONFIG_FONTENGINE_FREETYPE
  vid->alpha_charblit = &def_alpha_charblit;
#endif
#ifdef CONFIG_DITHER
  vid->dither_start = &def_dither_start;
  vid->dither_store = &def_dither_store;
  vid->dither_finish = &def_dither_finish;
#endif
}

/* The End */
