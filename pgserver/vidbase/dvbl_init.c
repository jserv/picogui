/* $Id: dvbl_init.c,v 1.2 2002/07/03 22:03:31 micahjd Exp $
 *
 * dvbl_init.c - This file is part of the Default Video Base Library,
 *               providing the basic video functionality in picogui but
 *               easily overridden by drivers.
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
 */

#include <pgserver/common.h>

#ifdef DEBUG_VIDEO
#define DEBUG_FILE
#endif
#include <pgserver/debug.h>

#include <pgserver/video.h>
#include <pgserver/font.h>
#include <pgserver/render.h>

/******* no-op functions */

g_error def_setmode(s16 xres,s16 yres,s16 bpp,u32 flags) {
  return success;
}

void def_font_newdesc(struct fontdesc *fd, const u8 *name, int size, int flags) {
}

void emulate_dos(void) {
}

g_error def_enterexitmode(void) {
   return success;
}

void def_update(s16 x,s16 y,s16 w,s16 h) {
}

void def_coord_logicalize(int *x,int *y) {
}

void def_coord_keyrotate(s16 *k) {
}

void def_font_sizetext_hook(struct fontdesc *fd, s16 *w, s16 *h, const u8 *txt) {
}

void def_font_outtext_hook(hwrbitmap *dest, struct fontdesc **fd,
			   s16 *x,s16 *y,hwrcolor *col,const u8 **txt,
			   struct quad **clip, s16 *lgop, s16 *angle) {
}

void def_font_outchar_hook(hwrbitmap *dest, struct fontdesc **fd,
			   s16 *x,s16 *y,hwrcolor *col,int *c,
			   struct quad **clip, s16 *lgop, s16 *angle) {
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

/******* registration */

void setvbl_default(struct vidlib *vid) {
  /* Set defaults */
  vid->color_pgtohwr = &def_color_pgtohwr;
  vid->color_hwrtopg = &def_color_hwrtopg;
  vid->font_newdesc = &def_font_newdesc;
  vid->slab = &def_slab;
  vid->bar = &def_bar;
  vid->line = &def_line;
  vid->rect = &def_rect;
  vid->gradient = &def_gradient;
  vid->charblit = &def_charblit;
  vid->tileblit = &def_tileblit;
  vid->scrollblit = &def_scrollblit;
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
  vid->bitmap_rotate90 = &def_bitmap_rotate90;
  vid->entermode = &def_enterexitmode;
  vid->exitmode = &def_enterexitmode;
  vid->bitmap_modeconvert = &def_bitmap_modeconvert;
  vid->bitmap_modeunconvert = &def_bitmap_modeunconvert;
  vid->bitmap_get_groprender = &def_bitmap_get_groprender;
  vid->coord_keyrotate = &def_coord_keyrotate;
  vid->font_getglyph = &def_font_getglyph;
  vid->font_sizetext_hook = &def_font_sizetext_hook;
  vid->font_outtext_hook = &def_font_outtext_hook;
  vid->font_outchar_hook = &def_font_outchar_hook;
  vid->grop_render_presetup_hook = &def_grop_render_presetup_hook;
  vid->grop_render_postsetup_hook = &def_grop_render_presetup_hook;
  vid->grop_render_end_hook = &def_grop_render_end_hook; 
  vid->grop_render_node_hook = &def_grop_render_node_hook;
  vid->update_hook = &def_update_hook;
  vid->grop_handler = &def_grop_handler;
  vid->blur = &def_blur;
#ifdef CONFIG_DITHER
  vid->dither_start = &def_dither_start;
  vid->dither_store = &def_dither_store;
  vid->dither_finish = &def_dither_finish;
#endif
}

/* The End */
