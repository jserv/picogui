/* $Id: sdlgl_util.c,v 1.7 2002/03/03 20:05:29 micahjd Exp $
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

  /***************** Background grid */

  if (gl_global.grid)
    gl_render_grid();

  /***************** Divtrees */

  /* Redraw the whole frame */
  for (p=dts->top;p;p=p->next)
    p->flags |= DIVTREE_ALL_REDRAW;
  update(NULL,0);

  /***************** Sprites */

  vid->sprite_showall();

  /***************** Onscreen display */

  /* FPS calculations */
  now = getticks();
  frames++;
  interval = (now-then)/1000.0f;
  if (interval > gl_global.fps_interval) {
    then = now;
    gl_global.fps = frames / interval;
    frames = 0;
  }

  gl_process_camera_keys();


  if (gl_global.showfps)
    gl_osd_printf(&i,"FPS: %.2f",gl_global.fps);

  switch (gl_global.camera_mode) {
  case SDLGL_CAMERAMODE_TRANSLATE:
    gl_osd_printf(&i,"Camera pan/zoom mode");
    break;
  case SDLGL_CAMERAMODE_ROTATE:
    gl_osd_printf(&i,"Camera rotate mode");
    break;
  }

  if (gl_global.grid)
    gl_osd_printf(&i,"Grid enabled");

  /***************** Done */

  gl_global.allow_update = 0;
  SDL_GL_SwapBuffers();
}

void gl_osd_printf(int *y, const char *fmt, ...) {
  char buf[256];
  va_list v;
  struct fontdesc *fd;
  s16 w,h;

  if (iserror(rdhandle((void**)&fd,PG_TYPE_FONTDESC,-1,gl_global.osd_font)))
    return;

  va_start(v,fmt);
  vsnprintf(buf,sizeof(buf),fmt,v);
  va_end(v);
  sizetext(fd,&w,&h,buf);

  /* Save the current matrix, set up a pixel coordinates matrix */
  glPushMatrix();
  glLoadIdentity();
  gl_matrix_pixelcoord();

  /* Draw a shaded background behind the OSD text */
  gl_lgop(PG_LGOP_ALPHA);
  glColor4f(0.0f,0.0f,0.0f,0.5f);
  glBegin(GL_QUADS);
  glVertex2f(5,5+*y);
  glVertex2f(5+w,5+*y);
  glVertex2f(5+w,5+*y+h);
  glVertex2f(5,5+*y+h);
  glEnd();
  
  outtext(vid->display,fd,5,5+*y,0xFFFF00,buf,NULL,PG_LGOP_NONE,0);

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
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  glClearDepth(1.0);

  /* Reset matrix */
  glPushMatrix();
  glLoadIdentity();
  gl_matrix_pixelcoord();

  /* Green mesh */
  glColor3f(0.0f, 0.5f, 0.0f);
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

/* The End */









