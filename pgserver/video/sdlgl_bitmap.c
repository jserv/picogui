/* $Id: sdlgl_bitmap.c,v 1.4 2002/05/22 09:26:33 micahjd Exp $
 *
 * sdlgl_bitmap.c - OpenGL driver for picogui, using SDL for portability
 *                  Functions to replace PicoGUI's normal bitmap data type
 *
 * Note: OpenGL doesn't have a portable way to render to an offscreen
 *       buffer, so this driver keeps offscreen surfaces as a 32bpp
 *       stdbitmap, accessed via the linear32 VBL. These surfaces are
 *       converted into OpenGL textures when necessary.
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

g_error sdlgl_bitmap_get_groprender(hwrbitmap bmp, struct groprender **rend) {
  struct glbitmap *glb = (struct glbitmap *) bmp;
  g_error e;
    
  /* Special case for the display */
  if (!bmp) {

    if (gl_global.display_rend) {
      *rend = gl_global.display_rend;
      return success;
    }

    e = g_malloc((void **) &gl_global.display_rend,sizeof(struct groprender));
    errorcheck;
    memset(gl_global.display_rend,0,sizeof(struct groprender));
    gl_global.display_rend->lgop = PG_LGOP_NONE;
    gl_global.display_rend->output = NULL;
    gl_global.display_rend->hfont = res[PGRES_DEFAULT_FONT];
    gl_global.display_rend->clip.x2 = vid->lxres - 1;
    gl_global.display_rend->clip.y2 = vid->lyres - 1;
    gl_global.display_rend->orig_clip = gl_global.display_rend->clip;
    gl_global.display_rend->output_rect.w = vid->lxres;
    gl_global.display_rend->output_rect.h = vid->lyres;
    *rend = gl_global.display_rend;
  }
  else {
    e = def_bitmap_get_groprender(glb->sb,rend);
    errorcheck;
  }

  (*rend)->output = bmp;

  return success;
}

g_error sdlgl_bitmap_getsize(hwrbitmap bmp,s16 *w,s16 *h) {
  struct glbitmap *glb = (struct glbitmap *) bmp;
  
  /* Special case for the display */
  if (!bmp) {
    *w = vid->xres;
    *h = vid->yres;
    return success;
  }

  return def_bitmap_getsize(glb->sb,w,h);
}

g_error sdlgl_bitmap_new(hwrbitmap *bmp,s16 w,s16 h,u16 bpp) {
  struct glbitmap **glb = (struct glbitmap **) bmp;
  g_error e;

  /* Allocate a glbitmap structure for this */
  e = g_malloc((void **) glb,sizeof(struct glbitmap));
  errorcheck;
  memset(*glb,0,sizeof(struct glbitmap));

  /* Allocate the stdbitmap */
  return def_bitmap_new(&(*glb)->sb,w,h,32);
}

void sdlgl_bitmap_free(hwrbitmap bmp) {
  struct glbitmap *glb = (struct glbitmap *) bmp;
  def_bitmap_free(glb->sb);
  if (glb->tile)
    sdlgl_bitmap_free((hwrbitmap)glb->tile);
  if (glb->texture)
    glDeleteTextures(1,&glb->texture);
  g_free(glb);
}

/* Delete the opengl texture associated with a
 * bitmap when the bitmap's contents change 
 */
int gl_invalidate_texture(hwrbitmap bit) {
  struct glbitmap *glb = (struct glbitmap *) bit;

  if (glb->tile) {
    vid->bitmap_free((hwrbitmap) glb->tile);
    glb->tile = NULL;
  }

  if (glb->texture) {
    glDeleteTextures(1,&glb->texture);
    glb->texture = 0;
  }

  return 1;
}

/* The End */









