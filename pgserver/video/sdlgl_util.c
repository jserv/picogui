/* $Id: sdlgl_util.c,v 1.22 2002/11/04 04:02:38 micahjd Exp $
 *
 * sdlgl_util.c - OpenGL driver for picogui, using SDL for portability.
 *                This file has utilities shared by multiple components of the driver.
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
#include <SDL/SDL_endian.h>

struct sdlgl_data gl_global;

inline void gl_color(hwrcolor c) {
  if (c & PGCF_ALPHA) {
    /* Convert 7-bit alpha channel to 8-bit */
    glColor4ub(getred(c),
	       getgreen(c),
	       getblue(c),
	       c>>23 | ((c>>24)&1));
  }
  else {
    glColor3ub(getred(c),
	       getgreen(c),
	       getblue(c));
  }
}

/* Measure the distance between (point_x,point_y) and a line
 * through (line_px,line_py) and paralell to the unit vector (line_vx,line_vy).
 */
inline float gl_dist_line_to_point(float point_x, float point_y, 
				   float line_px, float line_py,
				   float line_vx, float line_vy) {
  return (point_x - line_px)*line_vy - (point_y - line_py)*line_vx;
}

/* OpenGL can't support all of picogui's LGOPs, but try to
 * make good approximations of what the user probably intended.
 *
 * FIXME: most of these haven't been implemented/tested
 */
inline void gl_lgop(s16 lgop) {

  /* Normally disable blending for PG_LGOP_NONE, but if we're
   * antialiasing we always need to blend.
   */
  if (!gl_global.antialias) {
    if (lgop==PG_LGOP_NONE) {
      glDisable(GL_BLEND);
      return;
    }

    glEnable(GL_BLEND);
  }

  switch (lgop) {

  case PG_LGOP_NULL:
    glBlendFunc(GL_ZERO,GL_ONE);
    glBlendEquation(GL_FUNC_ADD);
    break;

  case PG_LGOP_XOR:
    break;

  case PG_LGOP_INVERT:
    break;

  case PG_LGOP_INVERT_OR:
    break;

  case PG_LGOP_INVERT_AND:
    break;

  case PG_LGOP_INVERT_XOR:
    break;

  case PG_LGOP_OR:
  case PG_LGOP_ADD:
    glBlendFunc(GL_ONE,GL_ONE);
    glBlendEquation(GL_FUNC_ADD);
    break;

  case PG_LGOP_SUBTRACT:
    glBlendFunc(GL_ONE,GL_ONE);
    glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
    break;

  case PG_LGOP_AND:
  case PG_LGOP_MULTIPLY:
    glBlendFunc(GL_ZERO,GL_SRC_COLOR);
    glBlendEquation(GL_FUNC_ADD);
    break;

  case PG_LGOP_STIPPLE:
    break;

    /* Note that it will only reach this point with PG_LGOP_NONE if
     * antialiasing is on, and in that case we have to blend everything
     */
  case PG_LGOP_NONE:
  case PG_LGOP_ALPHA:
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);
    break;

  }
}

/* Round up to the nearest power of two */
int gl_power2_round(int x) {
  int i,j;

  /* Zero is a special case.. */
  if (!x)
    return 0;

  /* Find the index of the highest bit set */
  for (i=0,j=x;j!=1;i++)
    j >>= 1;

  /* If that's the only bit set, the input was already
   * a power of two and we can leave it alone
   */
  if (x == 1<<i)
    return x;
  
  /* Otherwise get the next power of two */
  return 1<<(i+1);
}

/* Re-render the whole frame, keep track of frames per second */
void gl_frame(void) {
  struct divtree *p;
  static u32 frames = 0;
  static u32 then = 0;
  u32 now;
  float interval;
  int i = 0;

  gl_global.need_update = 0;
  gl_global.allow_update = 1;

  gl_matrix_camera();

  /***************** Background grid */

  glClear(GL_DEPTH_BUFFER_BIT);

  if (gl_global.grid)
    gl_render_grid();

  /***************** Divtrees */
  
  gl_set_wireframe(gl_global.wireframe);
  glEnable(GL_CLIP_PLANE0);
  glEnable(GL_CLIP_PLANE1);
  glEnable(GL_CLIP_PLANE2);
  glEnable(GL_CLIP_PLANE3);

  /* Redraw the whole frame */
  for (p=dts->top;p;p=p->next)
    p->flags |= DIVTREE_ALL_REDRAW;
  update(NULL,0);

  glDisable(GL_CLIP_PLANE0);
  glDisable(GL_CLIP_PLANE1);
  glDisable(GL_CLIP_PLANE2);
  glDisable(GL_CLIP_PLANE3);
  gl_set_wireframe(0);

  /***************** Sprites */

  vid->sprite_showall();

  /***************** Onscreen display */

  /* FPS calculations */
  now = os_getticks();
  frames++;
  interval = (now-then)/1000.0f;
  if (interval > gl_global.fps_interval) {
    then = now;
    gl_global.fps = frames / interval;
    frames = 0;
  }

  gl_process_camera_keys();
  gl_process_camera_smoothing();

  if (gl_global.showfps)
    gl_osd_printf(&i,"FPS: %.2f",gl_global.fps);

  switch (gl_global.camera_mode) {
  case SDLGL_CAMERAMODE_TRANSLATE:
    gl_osd_printf(&i,"Camera pan/zoom mode");
    break;
  case SDLGL_CAMERAMODE_ROTATE:
    gl_osd_printf(&i,"Camera rotate mode");
    break;
  case SDLGL_CAMERAMODE_FOLLOW_MOUSE:
    gl_osd_printf(&i,"Camera following mouse, shift-mousewheel to zoom");
    break;
  }

  if (gl_global.grid)
    gl_osd_printf(&i,"Grid enabled");

  if (gl_global.wireframe)
    gl_osd_printf(&i,"Wireframe mode");

  /***************** Done */

  gl_global.allow_update = 0;
  SDL_GL_SwapBuffers();
}

void gl_osd_printf(int *y, const char *fmt, ...) {
  char buf[256];
  va_list v;
  s16 w,h;
  struct pair xy;
  struct quad clip;

  va_start(v,fmt);
  vsnprintf(buf,sizeof(buf),fmt,v);
  va_end(v);
  gl_global.osd_font->lib->measure_string(gl_global.osd_font,pgstring_tmpwrap(buf),0,&w,&h);

  /* Save the current matrix, set up a pixel coordinates matrix */
  glPushMatrix();
  glLoadIdentity();
  gl_matrix_pixelcoord();

  /* Draw the text yellow, on a black drop shadow */
  clip.x1 = 0;
  clip.y1 = 0;
  clip.x2 = vid->lxres-1;
  clip.y2 = vid->lyres-1;
  xy.x = 8;
  xy.y = 3+(*y);  
  gl_global.osd_font->lib->draw_string(gl_global.osd_font,vid->display,
				       &xy,0x000000,pgstring_tmpwrap(buf),
				       &clip,PG_LGOP_NONE,0);
  xy.x = 5;
  xy.y = *y;  
  gl_global.osd_font->lib->draw_string(gl_global.osd_font,vid->display,
				       &xy,0xFFFF00,pgstring_tmpwrap(buf),
				       &clip,PG_LGOP_NONE,0);
  *y += h;

  /* Restore matrix */
  glPopMatrix();
}

void gl_matrix_pixelcoord(void) {
  glScalef(GL_SCALE,GL_SCALE,1.0f);
  glScalef(2.0f/vid->xres,-2.0f/vid->yres,1.0f);
  glTranslatef(-vid->xres/2,-vid->yres/2,-GL_DEFAULTDEPTH);
}

void gl_render_grid(void) {
  int i,j;

  /* Clear background */
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  /* Reset matrix */
  glPushMatrix();
  glLoadIdentity();
  gl_matrix_pixelcoord();

  /* Green mesh */
  glColor3f(0.0f, 0.6f, 0.0f);
  glBegin(GL_LINES);
  /* Horizontal lines. */
  for (i=0; i<=vid->yres; i+=32) {
    glVertex2f(0, i);
    glVertex2f(vid->xres, i);
  }
  /* Vertical lines. */
  for (i=0; i<=vid->xres; i+=32) {
    glVertex2f(i, 0);
    glVertex2f(i, vid->yres);
  }
  glEnd();

  glPopMatrix();
}

/* Show a texture on the screen and pause, for debugging
 */
void gl_showtexture(GLuint tex, int w, int h) {
  printf("Showing texture %d\n", tex);
  gl_lgop(PG_LGOP_NONE);
  glDisable(GL_TEXTURE_2D);
  glBegin(GL_QUADS);
  gl_color(0x000080);
  glVertex2f(0,0);
  glVertex2f(10000,0);
  glVertex2f(10000,10000);
  glVertex2f(0,10000);
  glEnd();
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, tex);
  glBegin(GL_QUADS);
  gl_color(0xFFFFFF);
  glNormal3f(0.0f,0.0f,1.0f);
  glTexCoord2f(0,0);
  glVertex2f(0,0);
  glTexCoord2f(1,0);
  glVertex2f(w,0);
  glTexCoord2f(1,1);
  glVertex2f(w,h);
  glTexCoord2f(0,1);
  glVertex2f(0,h);
  glEnd();
  glDisable(GL_TEXTURE_2D);
  SDL_GL_SwapBuffers();
  sleep(2);
}

void gl_make_texture(struct glbitmap *glb) {

  /* FIXME: This is a hack! We need lock/unlock commands for SHM bitmaps so we know when to update. This
   * code just updates volatile bitmaps every blit, limiting the maximum number of updates per second.
   */
  if (glb->volatilesb && os_getticks()>glb->last_update_time+30) {
    gl_invalidate_texture((hwrbitmap)glb);
    glb->last_update_time = os_getticks();
  }

  if (!glb->texture) {
    hwrbitmap tmpbit;
    u32 *p;
    u32 i;
    
    glGenTextures(1,&glb->texture);
    glBindTexture(GL_TEXTURE_2D, glb->texture);
    
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,gl_global.texture_filtering);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,gl_global.texture_filtering);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
    
    /* We have to round up to the nearest power of two...
     * FIXME: this is wasteful. We need a way to pack multiple
     *        glbitmaps into one texture.
     */
    glb->tw = gl_power2_round(glb->sb->w);
    glb->th = gl_power2_round(glb->sb->h);
    glb->tx1 = 0;
    glb->ty1 = 0;
    glb->tx2 = ((float)glb->sb->w) / ((float)glb->tw);
    glb->ty2 = ((float)glb->sb->h) / ((float)glb->th);
    
    /* Copy the image to a temporary bitmap.
     * Duplicate edges if we have room, to simulate texture clamping there.
     */
    if (iserror(vid->bitmap_new(&tmpbit, glb->tw, glb->th, vid->bpp))) return;
    vid->blit(tmpbit, 0,0, glb->sb->w, glb->sb->h, (hwrbitmap)glb, 0,0, PG_LGOP_NONE);

    /* Left and right edges */
    if (glb->tw > glb->sb->w) {
      vid->blit(tmpbit, glb->sb->w,0, 1, glb->sb->h, (hwrbitmap)glb, glb->sb->w-1,0, PG_LGOP_NONE);
      vid->blit(tmpbit, glb->tw-1,0, 1,glb->sb->h, (hwrbitmap)glb, 0,0, PG_LGOP_NONE);
    }
    
    /* Top and bottom edges */
    if (glb->th > glb->sb->h) {
      vid->blit(tmpbit, 0,glb->sb->h, glb->sb->w, 1, (hwrbitmap)glb, 0,glb->sb->h-1, PG_LGOP_NONE);
      vid->blit(tmpbit, 0,glb->th-1, glb->sb->w,1, (hwrbitmap)glb, 0,0, PG_LGOP_NONE);
    }
    
    /* Corners */
    if (glb->tw > glb->sb->w && glb->th > glb->sb->h) {
      vid->blit(tmpbit, glb->sb->w,glb->sb->h, 1, 1, (hwrbitmap)glb, 
		glb->sb->w-1,glb->sb->h-1, PG_LGOP_NONE);
      vid->blit(tmpbit, glb->tw-1,glb->th-1, 1,1, (hwrbitmap)glb, 0,0, PG_LGOP_NONE);
    }

    /* Now convert the alpha channel in our temporary bitmap. Any colors with the
     * PGCF_ALPHA flag need to have their alpha channels shifted over one bit,
     * other pixels get an alpha of 0xFF. Note that the original bitmap will still
     * have the PGCF_ALPHA-style colors, so detection of bitmaps with alpha channels
     * will work as usual.
     */
    i = glb->tw * glb->th;
    p = (u32 *)((struct glbitmap*)tmpbit)->sb->bits;
    for (;i;i--,p++) {
      if (*p & PGCF_ALPHA) {
	*p = (*p & 0x1FFFFFF) | ((*p & 0xFF000000)<<1);
      }
      else
	*p = *p | 0xFF000000;


      /* On big-endian machines we need to swap it into BGRA order..
       * It's an OpenGL quirk
       */
      *p = SDL_SwapLE32(*p);
    }
    
    /* Send it to opengl, ditch our temporary */
    glTexImage2D(GL_TEXTURE_2D, 0, 4, glb->tw, glb->th, 0, 
		 GL_BGRA_EXT, GL_UNSIGNED_BYTE, ((struct glbitmap*)tmpbit)->sb->bits);
    vid->bitmap_free(tmpbit);
    
    //  gl_showtexture(glb->texture, glb->tw, glb->th);
  }      
}

void gl_set_wireframe(int on) {
  glPolygonMode(GL_FRONT_AND_BACK, on ? GL_LINE : GL_FILL);
}

/* The End */









