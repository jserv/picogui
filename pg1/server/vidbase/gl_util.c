/* $Id$
 *
 * gl_util.c - OpenGL driver for picogui
 *             This file has utilities shared by multiple components of the driver.
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

struct gl_data gl_global;

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

  gl_global.need_update = 0;
  gl_global.allow_update = 1;
  gl_frame_setup();

  /* Reset OSD */
  gl_global.osd_y = 0;

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
    gl_osd_printf("FHz: %.2f",gl_global.fps);

  switch (gl_global.camera_mode) {
  case GL_CAMERAMODE_TRANSLATE:
    gl_osd_printf("Camera pan/zoom mode");
    break;
  case GL_CAMERAMODE_ROTATE:
    gl_osd_printf("Camera rotate mode");
    break;
  case GL_CAMERAMODE_FOLLOW_MOUSE:
    gl_osd_printf("Camera following mouse, shift-mousewheel to zoom");
    break;
  }

  if (gl_global.grid)
    gl_osd_printf("Grid enabled");

  if (gl_global.wireframe)
    gl_osd_printf("Wireframe mode");

  /***************** Done */

  gl_global.allow_update = 0;
  vid->update(vid->display,0,0,vid->xres,vid->yres);
  gl_frame_cleanup();
}

void gl_frame_setup(void) {
  /* Push current modelview and projection matrices,
   * Set up our perspective pixel-coordinates matrices.
   */
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  gluPerspective(GL_FOV,1,GL_MINDEPTH,GL_MAXDEPTH*2);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
  gl_matrix_camera();

  /* Make sure appropriate modes are set */
  if (gl_global.antialias) {
    glEnable(GL_POLYGON_SMOOTH);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_BLEND);
  }
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_LIGHTING);
  glDisable(GL_SMOOTH);
  glDisable(GL_TEXTURE_2D);

  /* We have no idea what the current texture is now */
  gl_global.current_texture = -1;
}

void gl_frame_cleanup(void) {
  /* Pop matrices */
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();

  /* Restore sane defaults */
  if (gl_global.antialias) {
    glDisable(GL_POLYGON_SMOOTH);
    glDisable(GL_LINE_SMOOTH);
  }
  glDisable(GL_BLEND);
  glDisable(GL_TEXTURE_2D);
}

void gl_osd_printf(const char *fmt, ...) {
  char buf[256];
  va_list v;
  s16 w,h;
  struct pgpair xy;
  struct pgquad clip;
  struct font_style fs;

  /* Load a font for the OSD */
  if (!gl_global.osd_font) {
    memset(&fs,0,sizeof(fs));
    fs.size = get_param_int(GL_SECTION,"osd_fontsize",20);
    fs.style = PG_FSTYLE_BOLD;
    if (iserror(font_descriptor_create(&gl_global.osd_font,&fs)))
      return;
  }

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
  xy.x = 6;
  xy.y = 6 + gl_global.osd_y;  
  gl_global.osd_font->lib->draw_string(gl_global.osd_font,vid->display,
				       &xy,0x000000,pgstring_tmpwrap(buf),
				       &clip,PG_LGOP_NONE,0);
  xy.x = 5;
  xy.y = 5 + gl_global.osd_y;  
  gl_global.osd_font->lib->draw_string(gl_global.osd_font,vid->display,
				       &xy,0xFFFF00,pgstring_tmpwrap(buf),
				       &clip,PG_LGOP_NONE,0);
  gl_global.osd_y += h;

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
  glClearColor(0.0f, 0.4f, 0.0f, 0.0f);
  glClearDepth(1.0);
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


void gl_bind_texture(struct glbitmap *glb) {
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
    s32 i;
    
    glGenTextures(1,&glb->texture);
    glBindTexture(GL_TEXTURE_2D, glb->texture);
    gl_global.current_texture = glb->texture;

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
    for (i=glb->tw - glb->sb->w - 1;i>=0;i--)
      vid->blit(tmpbit, glb->sb->w+i,0, 1, glb->sb->h, (hwrbitmap)glb, glb->sb->w-1,0, PG_LGOP_NONE);
    
    /* Top and bottom edges */
    for (i=glb->th - glb->sb->h - 1;i>=0;i--)
      vid->blit(tmpbit, 0,glb->sb->h+i, glb->sb->w,1, (hwrbitmap)glb, 0,glb->sb->h-1, PG_LGOP_NONE);
    
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


#if __BYTE_ORDER == __BIG_ENDIAN
      /* OpenGL requires that the pixels are always in little-endian */
      *p  = (((*p)&0xFF000000)>>24) |
	    (((*p)&0x00FF0000)>>8)  |
 	    (((*p)&0x0000FF00)<<8)  |
	    (((*p)&0x000000FF)<<24) ;
#endif
    }
    
    /* Send it to opengl, ditch our temporary */
    gluBuild2DMipmaps(GL_TEXTURE_2D, 4, glb->tw, glb->th,
		      GL_BGRA, GL_UNSIGNED_BYTE, ((struct glbitmap*)tmpbit)->sb->bits);
    vid->bitmap_free(tmpbit);
  }      
  else {
    /* Texture already exists, just bind it */
    if (glb->texture!=gl_global.current_texture) {
      glBindTexture(GL_TEXTURE_2D, glb->texture);
      gl_global.current_texture = glb->texture;
    }
  }   

}

void gl_set_wireframe(int on) {
  glPolygonMode(GL_FRONT_AND_BACK, on ? GL_LINE : GL_FILL);
}

/* Tiled feedback blitter, the basis for blurring and normal feedback effects */
void gl_feedback(int x, int y, int w, int h, int lgop, int filter,
		 int source, int generate_mipmaps, int mipmap_level) {
  /* Texture size (256x256) */
  const int t_exponent = 8;
  const int t_size = 1 << t_exponent;
  static GLuint texture = 0;
  float texw, texh;
  int tilex, tiley, tilew, tileh;
  int i,j,ix,iy;
  int h_tiles = (w + t_size - 1) >> t_exponent;
  int v_tiles = (h + t_size - 1) >> t_exponent;
  float originx, originy, originw;
  float projection[16], modelview[16];

  /* We'll need a temporary texture if we don't already have one */
  if (!texture) {
    glGenTextures(1,&texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,filter);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,filter);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);

    /* Just allocate an empty texture */
    glTexImage2D(GL_TEXTURE_2D, 0, 3, t_size, t_size, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
  }
  else {
    glBindTexture(GL_TEXTURE_2D, texture);
  }

  /* Use an OpenGL extension to automatically generate mipmaps if we want that */
  glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP_SGIS, generate_mipmaps);
  glEnable(GL_TEXTURE_2D);
  glReadBuffer(source);

  /* Find the origin of the modelview matrix, projected into viewport coordinates */
  glGetFloatv(GL_PROJECTION_MATRIX, projection);
  glGetFloatv(GL_MODELVIEW_MATRIX, modelview);
  /* First multiply the modelview matrix's rightmost column (the origin)
   * by the projection matrix. We ignore Z, and get x, y, and w.
   */
  originx  = modelview[12]*projection[0]  + modelview[13]*projection[4] + 
             modelview[14]*projection[8]  + modelview[15]*projection[12];
  originy  = modelview[12]*projection[1]  + modelview[13]*projection[5] + 
             modelview[14]*projection[9]  + modelview[15]*projection[13];
  originw  = modelview[12]*projection[3]  + modelview[13]*projection[7] + 
             modelview[14]*projection[11] + modelview[15]*projection[15];
  /* Division for the perspective calculation */
  originx /= originw;
  originy /= originw;
  /* Convert from normalized device coordinates to pixels */
  originx = (originx*0.5+0.5) * vid->xres;
  originy = (-originy*0.5+0.5) * vid->yres;

  /* Split into tiles no larger than the texture size */
  for (j=0,iy=0;j<v_tiles;j++,iy+=t_size)
    for (i=0,ix=0;i<h_tiles;i++,ix+=t_size) {
      tilex = x+ix;
      tiley = y+iy;
      tilew = min(w-ix,t_size);
      tileh = min(h-iy,t_size);

      /* Copy from the source buffer */
      glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_BASE_LEVEL,0);
      glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 
			  tilex+originx, vid->yres-(originy+tiley+tileh), tilew, tileh);

      glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_BASE_LEVEL,mipmap_level);

      texw = ((float)tilew) / t_size;
      texh = ((float)tileh) / t_size;
      gl_lgop(lgop);
      
      /* Render from texture */
      glBegin(GL_QUADS);
      glColor4f(1.0f,1.0f,1.0f,1.0f);
      glNormal3f(0.0f,0.0f,1.0f);
      glTexCoord2f(0,texh);
      glVertex2f(tilex,tiley);
      glTexCoord2f(texw,texh);
      glVertex2f(tilex+tilew,tiley);
      glTexCoord2f(texw,0);
      glVertex2f(tilex+tilew,tiley+tileh);
      glTexCoord2f(0,0);
      glVertex2f(tilex,tiley+tileh);
      glEnd();
    }
  
  glDisable(GL_TEXTURE_2D);
}

/* The End */









