/* $Id: sdlgl.c,v 1.9 2002/03/02 05:04:28 micahjd Exp $
 *
 * sdlgl.c - OpenGL driver for picogui, using SDL for portability
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

#include <pgserver/common.h>      /* Needed for any pgserver file */

#include <pgserver/video.h>       /* Always needed for a video driver! */
#include <pgserver/render.h>      /* For data types like 'quad' */
#include <pgserver/input.h>       /* For loading our corresponding input lib */
#include <pgserver/configfile.h>  /* For loading our configuration */
#include <pgserver/appmgr.h>      /* Default font */
#include <pgserver/font.h>        /* font rendering */

#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>

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

#include <math.h>       /* Floating point math in picogui? Blasphemy :) */
#include <stdarg.h>     /* for gl_osd_printf */
#include <string.h>

/************************************************** Definitions */

/* Perspective values */
#define GL_FOV          45      /* Vertical field of view in degrees */
#define GL_MINDEPTH     0.01
#define GL_MAXDEPTH     (vid->xres*2)
#define GL_DEFAULTDEPTH (vid->xres)
#define GL_SCALE        (tan(GL_FOV/360.0f*3.141592654)*GL_DEFAULTDEPTH)

SDL_Surface *sdlgl_vidsurf;

#ifdef CONFIG_SDLSKIN
extern s16 sdlfb_display_x;
extern s16 sdlfb_display_y;
extern u16 sdlfb_scale;
#endif

/* Type of texture filtering to use */
GLint gl_texture_filtering;

/* Frames per second, calculated in gl_frame() */
float gl_fps;

/* FPS calculation interval. Larger values are more accurate, smaller
 * values update the FPS value faster. Units are seconds.
 */
float gl_fps_interval;

/* Input library optionally loaded to make this driver continously redraw */
struct inlib *gl_continuous;

/* Redefine PicoGUI's hwrbitmap
 * Note that it's very likely we'll want to use this as a texture, yet
 * OpenGL can't render directly into textures. So, we'll keep a stdbitmap
 * representation that we can draw on with linear32, and if necessary
 * we also have an OpenGL texture for quick blitting. The OpenGL texture
 * is only created when needed, and it is destroyed if the underlying
 * stdbitmap is modified.
 */
struct glbitmap {
  struct stdbitmap *sb;

  /* OpenGL texture, valid if texture is nonzero */
  GLuint texture;
  float tx1,ty1,tx2,ty2;  /* Texture coordinates */
  int tw,th;              /* Texture size (power of two) */

  /* If non-NULL, this is a cached tiled representation  */
  struct glbitmap *tile;
};

/* Like X11 and most other high-level graphics APIs, there is a relatively
 * large per-blit penalty. This becomes a big problem when drawing tiled
 * bitmaps. OpenGL natively handles tiling textures, but that doesn't help
 * us much since textures have to be a power of two in size. We solve this
 * by storing a pre-tiled copy of the bitmap. Bitmaps below the following size
 * are tiled and stored in the bitmap's 'tile' variable. This size should
 * be a power of two.
 */
#define GL_TILESIZE 256

/* Texture and coordinates for one glyph. A table of these is in the font's
 * "bitmaps" array.
 */
struct gl_glyph {
  GLuint texture;
  float tx1,ty1,tx2,ty2;  /* Texture coordinates */
};

/* Size of the textures to use for font conversion, in pixels. MUST be a power of 2 */
#define GL_FONT_TEX_SIZE 512

/* Macro to determine when to redirect drawing to linear32 */
#define GL_LINEAR32(dest) (dest)

/* Retrieve the associated stdbitmap for linear32 to operate on */
#define STDB(dest) (((struct glbitmap*)(dest))->sb)

/* Groprender structure for the display */
struct groprender *gl_display_rend = NULL;

/* Camera modes that let the user move the camera around */
int gl_camera_mode;
#define SDLGL_CAMERAMODE_NONE      0
#define SDLGL_CAMERAMODE_TRANSLATE 1
#define SDLGL_CAMERAMODE_ROTATE  2

/* For the camera movement, keep track of which keys are pressed */
u8 gl_pressed_keys[PGKEY_MAX];

/* Font for onscreen display */
handle gl_osd_font;

/* More flags */
int gl_grid;
int gl_showfps;

/* save the old font list so we can restore it on exit */
struct fontstyle_node *gl_old_fonts;

inline void gl_color(hwrcolor c);
inline float gl_dist_line_to_point(float point_x, float point_y, 
				   float line_px, float line_py,
				   float line_vx, float line_vy);
inline void gl_lgop(s16 lgop);
int gl_power2_round(int x);
void gl_frame(void);
void sdlgl_pixel(hwrbitmap dest,s16 x,s16 y,hwrcolor c,s16 lgop);
hwrcolor sdlgl_getpixel(hwrbitmap dest,s16 x,s16 y);
void sdlgl_update(s16 x, s16 y, s16 w, s16 h);
void sdlgl_rect(hwrbitmap dest,s16 x,s16 y,s16 w, s16 h, hwrcolor c,s16 lgop);
void sdlgl_slab(hwrbitmap dest,s16 x,s16 y,s16 w, hwrcolor c,s16 lgop);
void sdlgl_bar(hwrbitmap dest,s16 x,s16 y,s16 h, hwrcolor c,s16 lgop);
void sdlgl_line(hwrbitmap dest,s16 x1,s16 y1,s16 x2,s16 y2,hwrcolor c,s16 lgop);
void sdlgl_gradient(hwrbitmap dest,s16 x,s16 y,s16 w,s16 h,s16 angle,
		    pgcolor c1, pgcolor c2, s16 lgop);
g_error sdlgl_init(void);
g_error sdlgl_setmode(s16 xres,s16 yres,s16 bpp,u32 flags);
void sdlgl_close(void);
g_error sdlgl_regfunc(struct vidlib *v);
g_error sdlgl_bitmap_get_groprender(hwrbitmap bmp, struct groprender **rend);
g_error sdlgl_bitmap_getsize(hwrbitmap bmp,s16 *w,s16 *h);
g_error sdlgl_bitmap_new(hwrbitmap *bmp,s16 w,s16 h,u16 bpp);
void sdlgl_bitmap_free(hwrbitmap bmp);
void gl_continuous_init(int *n,fd_set *readfds,struct timeval *timeout);
g_error gl_continuous_regfunc(struct inlib *i);
void gl_osd_printf(int *y, const char *fmt, ...);
void gl_matrix_pixelcoord(void);
int sdlgl_key_event_hook(u32 *type, s16 *key, s16 *mods);
int sdlgl_pointing_event_hook(u32 *type, s16 *x, s16 *y, s16 *btn);
void gl_process_camera_keys(void);
void gl_render_grid(void);
g_error gl_load_font_style(TTF_Font *ttf, struct font **f, int style);
g_error gl_load_font(const char *file);
void sdlgl_font_newdesc(struct fontdesc *fd, const u8 *name, int size, int flags);
void sdlgl_font_outtext_hook(hwrbitmap *dest, struct fontdesc **fd,
			     s16 *x,s16 *y,hwrcolor *col,const u8 **txt,
			     struct quad **clip, s16 *lgop, s16 *angle);
void sdlgl_font_sizetext_hook(struct fontdesc *fd, s16 *w, s16 *h, const u8 *txt);
 
/************************************************** Utilities */

inline void gl_color(hwrcolor c) {
  glColor3ub(getred(c),
	     getgreen(c),
	     getblue(c));
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
  switch (lgop) {

  case PG_LGOP_NULL:
    glEnable(GL_BLEND);
    glBlendFunc(GL_ZERO,GL_ONE);
    glBlendEquation(GL_FUNC_ADD);
    break;

  case PG_LGOP_NONE:
    glDisable(GL_BLEND);
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
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE,GL_ONE);
    glBlendEquation(GL_FUNC_ADD);
    break;

  case PG_LGOP_SUBTRACT:
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE,GL_ONE);
    glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
    break;

  case PG_LGOP_AND:
  case PG_LGOP_MULTIPLY:
    glEnable(GL_BLEND);
    glBlendFunc(GL_ZERO,GL_SRC_COLOR);
    glBlendEquation(GL_FUNC_ADD);
    break;

  case PG_LGOP_STIPPLE:
    break;

  case PG_LGOP_ALPHA:
    glEnable(GL_BLEND);
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

  if (gl_grid)
    gl_render_grid();

  /* Redraw the whole frame */
  for (p=dts->top;p;p=p->next)
    p->flags |= DIVTREE_ALL_REDRAW;
  update(NULL,1);

  /* FPS calculations */
  now = getticks();
  frames++;
  interval = (now-then)/1000.0f;
  if (interval > gl_fps_interval) {
    then = now;
    gl_fps = frames / interval;
    frames = 0;
  }

  gl_process_camera_keys();
}

void gl_osd_printf(int *y, const char *fmt, ...) {
  char buf[256];
  va_list v;
  struct fontdesc *fd;
  int i,j;

  if (iserror(rdhandle((void**)&fd,PG_TYPE_FONTDESC,-1,gl_osd_font)))
    return;

  va_start(v,fmt);
  vsnprintf(buf,sizeof(buf),fmt,v);
  va_end(v);

  /* Save the current matrix, set up a pixel coordinates matrix */
  glPushMatrix();
  glLoadIdentity();
  gl_matrix_pixelcoord();

  outtext(vid->display,fd,5,5+*y,0xFFFF00,buf,NULL,PG_LGOP_NONE,0);

  /* 1.5 spacing between items */
  *y += fd->font->h + (fd->font->h>>1);

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

/************************************************** Basic primitives */

void sdlgl_pixel(hwrbitmap dest,s16 x,s16 y,hwrcolor c,s16 lgop) {
  if (GL_LINEAR32(dest)) {
    linear32_pixel(STDB(dest),x,y,c,lgop);
    return;
  }

  gl_lgop(lgop);
  glBegin(GL_QUADS);
  gl_color(c);
  glVertex2f(x,y);
  glVertex2f(x+1,y);
  glVertex2f(x+1,y+1);
  glVertex2f(x,y+1);
  glEnd();
}

/* This is _really_ damn slow, like the X11 getpixel,
 * but with all this fun hardware acceleration we shouldn't
 * actually have to use it much.
 */
hwrcolor sdlgl_getpixel(hwrbitmap dest,s16 x,s16 y) {
  u8 r,g,b;

  if (GL_LINEAR32(dest))
    return linear32_getpixel(STDB(dest),x,y);

  glReadPixels(x,y,1,1,GL_RED,GL_UNSIGNED_BYTE,&r);
  glReadPixels(x,y,1,1,GL_GREEN,GL_UNSIGNED_BYTE,&g);
  glReadPixels(x,y,1,1,GL_BLUE,GL_UNSIGNED_BYTE,&b);
  return mkcolor(r,g,b);
}

/*
 * This driver uses a game-like rendering approach of always drawing
 * onscreen displays before swapping the buffers. This almost always
 * means a lot of wasted drawing, but OpenGL is designed for that.
 */
void sdlgl_update(s16 x, s16 y, s16 w, s16 h) {
  int i = 0;

  if (gl_showfps)
    gl_osd_printf(&i,"FPS: %.2f",gl_fps);

  switch (gl_camera_mode) {
  case SDLGL_CAMERAMODE_TRANSLATE:
    gl_osd_printf(&i,"Camera pan/zoom mode");
    break;
  case SDLGL_CAMERAMODE_ROTATE:
    gl_osd_printf(&i,"Camera rotate mode");
    break;
  }

  if (gl_grid)
    gl_osd_printf(&i,"Grid enabled");

  SDL_GL_SwapBuffers();
};  

void sdlgl_rect(hwrbitmap dest,s16 x,s16 y,s16 w, s16 h, hwrcolor c,s16 lgop) {
  if (GL_LINEAR32(dest)) {
    linear32_rect(STDB(dest),x,y,w,h,c,lgop);
    return;
  }

  gl_lgop(lgop);
  glBegin(GL_QUADS);
  gl_color(c);
  glVertex2f(x,y);
  glVertex2f(x+w,y);
  glVertex2f(x+w,y+h);
  glVertex2f(x,y+h);
  glEnd();
}

void sdlgl_slab(hwrbitmap dest,s16 x,s16 y,s16 w, hwrcolor c,s16 lgop) {
  if (GL_LINEAR32(dest)) {
    linear32_slab(STDB(dest),x,y,w,c,lgop);
    return;
  }

  gl_lgop(lgop);
  glBegin(GL_QUADS);
  gl_color(c);
  glVertex2f(x,y);
  glVertex2f(x+w,y);
  glVertex2f(x+w,y+1);
  glVertex2f(x,y+1);
  glEnd();
}

void sdlgl_bar(hwrbitmap dest,s16 x,s16 y,s16 h, hwrcolor c,s16 lgop) {
  if (GL_LINEAR32(dest)) {
    linear32_bar(STDB(dest),x,y,h,c,lgop);
    return;
  }

  gl_lgop(lgop);
  glBegin(GL_QUADS);
  gl_color(c);
  glVertex2f(x,y);
  glVertex2f(x+1,y);
  glVertex2f(x+1,y+h);
  glVertex2f(x,y+h);
  glEnd();
}

void sdlgl_line(hwrbitmap dest,s16 x1,s16 y1,s16 x2,s16 y2,hwrcolor c,s16 lgop) {
  if (GL_LINEAR32(dest)) {
    linear32_line(STDB(dest),x1,y1,x2,y2,c,lgop);
    return;
  }

  gl_lgop(lgop);
  glBegin(GL_LINES);
  gl_color(c);
  glVertex2f(x1,y1);
  glVertex2f(x2,y2);
  glEnd();
}

/* Yes indeedy, hardware accelerated gradients!
 *
 * Since we're free to use fancy floating point math, this gradient
 * function uses a different algorithm than def_gradient. A line of
 * equal color exists perpendicular to the gradient angle. This line L
 * passes through the center of the gradient's rectangle. All color
 * is a function of the distance from this line. We scale it so the colors
 * given will match the corners of the rectangle farthest from the
 * perpendicularl line.
 *
 * And then the fun part- let OpenGL do all the interpolation for us!
 */
void sdlgl_gradient(hwrbitmap dest,s16 x,s16 y,s16 w,s16 h,s16 angle,
		  pgcolor c1, pgcolor c2, s16 lgop) {
  float theta;
  float r_v1,g_v1,b_v1;
  float r_v2,g_v2,b_v2;
  float r_c,g_c,b_c;
  float r_s,g_s,b_s;
  float vx,vy;
  float cx,cy;
  float farthest, d1, d2, d3, d4;

  if (GL_LINEAR32(dest)) {
    linear32_gradient(STDB(dest),x,y,w,h,angle,c1,c2,lgop);
    return;
  }

  gl_lgop(lgop);
  
  /* Convert angle to radians */
  theta = angle / 360.0f * 3.141592654 * 2.0f;

  /* Decode colors */
  r_v1 = getred(c1)/255.0f;
  g_v1 = getgreen(c1)/255.0f;
  b_v1 = getblue(c1)/255.0f;
  r_v2 = getred(c2)/255.0f;
  g_v2 = getgreen(c2)/255.0f;
  b_v2 = getblue(c2)/255.0f;
  
  /* Calculate center color */
  r_c = (r_v1 + r_v2)/2;
  g_c = (g_v1 + g_v2)/2;
  b_c = (b_v1 + b_v2)/2;

  /* Find a unit vector perpendicular to the specified angle */
  vx = sin(theta); 
  vy = cos(theta); 

  /* Find the center of the rectangle */
  cx = x + (w/2);
  cy = y + (h/2);

  /* Find the distance of the corner farthest from the center line */
  farthest =              d1 = gl_dist_line_to_point( x  ,y  , cx,cy,vx,vy );
  farthest = max(farthest,d2 = gl_dist_line_to_point( x+w,y  , cx,cy,vx,vy ));
  farthest = max(farthest,d3 = gl_dist_line_to_point( x+w,y+h, cx,cy,vx,vy ));
  farthest = max(farthest,d4 = gl_dist_line_to_point( x  ,y+h, cx,cy,vx,vy ));

  /* Find multipliers to convert distance from the line to distance
   * from the center color
   */
  r_s = (r_v2 - r_c) / farthest;
  g_s = (g_v2 - g_c) / farthest;
  b_s = (b_v2 - b_c) / farthest;
  
  /* Find the corresponding color for each vertex, let OpenGL interpolate.
   */
  glBegin(GL_QUADS);

  glColor3f(r_c + r_s * d1,
	    g_c + g_s * d1,
	    b_c + b_s * d1);
  glVertex2f(x,y);

  glColor3f(r_c + r_s * d2,
	    g_c + g_s * d2,
	    b_c + b_s * d2);
  glVertex2f(x+w,y);

  glColor3f(r_c + r_s * d3,
	    g_c + g_s * d3,
	    b_c + b_s * d3);
  glVertex2f(x+w,y+h);

  glColor3f(r_c + r_s * d4,
	    g_c + g_s * d4,
	    b_c + b_s * d4);
  glVertex2f(x,y+h);

  glEnd();  
}

void sdlgl_blit(hwrbitmap dest, s16 x,s16 y,s16 w,s16 h, hwrbitmap src,
		s16 src_x, s16 src_y, s16 lgop) {

  if (GL_LINEAR32(dest) && GL_LINEAR32(src)) {
    linear32_blit(STDB(dest),x,y,w,h,STDB(src),src_x,src_y,lgop);
    return;
  }

  /* Blitting from an offscreen bitmap to the screen? */
  if (GL_LINEAR32(src)) {
    struct glbitmap *glsrc = (struct glbitmap *) src;
    float tx1,ty1,tx2,ty2;

    /* If we're tiling, create a cached tiled bitmap at the minimum
     * size if necessary, and use that to reduce the number of separate
     * quads we have to send to OpenGL
     */
    if ((w > glsrc->sb->w || h > glsrc->sb->h) && 
	(glsrc->sb->w < GL_TILESIZE || glsrc->sb->h < GL_TILESIZE)) {
      /* Create a pre-tiled image */
      if (!glsrc->tile) {
	vid->bitmap_new( (hwrbitmap*)&glsrc->tile,
			 (GL_TILESIZE/glsrc->sb->w + 1) * glsrc->sb->w,
			 (GL_TILESIZE/glsrc->sb->h + 1) * glsrc->sb->h, vid->bpp );
	def_blit((hwrbitmap)glsrc->tile,0,0,glsrc->tile->sb->w,
		 glsrc->tile->sb->h,src,0,0,PG_LGOP_NONE);

	printf("Expand tile to %d,%d\n",glsrc->tile->sb->w,glsrc->tile->sb->h);
      }
      glsrc = glsrc->tile;
    }

    /* If we still have to tile, let defaulvbl do it
     */
    if (w > glsrc->sb->w || h > glsrc->sb->h)
      def_blit(dest,x,y,w,h,(hwrbitmap) glsrc,src_x,src_y,lgop);


    /* Make sure the bitmap has a corresponding texture */
    if (!glsrc->texture) {
      glGenTextures(1,&glsrc->texture);
      glBindTexture(GL_TEXTURE_2D, glsrc->texture);

      glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,gl_texture_filtering);
      glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,gl_texture_filtering);

      /* We have to round up to the nearest power of two...
       * FIXME: this is wasteful. We need a way to pack multiple
       *        glbitmaps into one texture.
       */
      glsrc->tw = gl_power2_round(glsrc->sb->w);
      glsrc->th = gl_power2_round(glsrc->sb->h);
      glsrc->tx1 = 0;
      glsrc->ty1 = 0;
      glsrc->tx2 = ((float)glsrc->sb->w) / ((float)glsrc->tw);
      glsrc->ty2 = ((float)glsrc->sb->h) / ((float)glsrc->th);
      
      /* Allocate texture */
      glTexImage2D(GL_TEXTURE_2D, 0, 4, glsrc->tw, glsrc->th, 0, 
		   GL_RGBA, GL_UNSIGNED_BYTE, NULL);

      /* Paste our subimage into it.
       * We put additional copies offset one pixel so that the row or column
       * immediately past the bitmap has the same data as the edge of the
       * bitmap, to eliminate rendering artifacts
       */
      glTexSubImage2D(GL_TEXTURE_2D,0, 1,0, glsrc->sb->w, glsrc->sb->h,
		      GL_BGRA_EXT, GL_UNSIGNED_BYTE, glsrc->sb->bits);
      glTexSubImage2D(GL_TEXTURE_2D,0, 0,1, glsrc->sb->w, glsrc->sb->h,
		      GL_BGRA_EXT, GL_UNSIGNED_BYTE, glsrc->sb->bits);
      glTexSubImage2D(GL_TEXTURE_2D,0, 0,0, glsrc->sb->w, glsrc->sb->h,
		      GL_BGRA_EXT, GL_UNSIGNED_BYTE, glsrc->sb->bits);

    }      
    
    /* Calculate texture coordinates */
    tx1 = glsrc->tx1 + src_x * (glsrc->tx2 - glsrc->tx1) / w;
    ty1 = glsrc->ty1 + src_y * (glsrc->ty2 - glsrc->ty1) / h;
    tx2 = glsrc->tx1 + (src_x + w) * (glsrc->tx2 - glsrc->tx1) / w;
    ty2 = glsrc->ty1 + (src_y + h) * (glsrc->ty2 - glsrc->ty1) / h;
    
    /* Draw a texture-mapped quad
     */
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, glsrc->texture);
    glBegin(GL_QUADS);
    glColor3f(1.0f,1.0f,1.0f);
    glNormal3f(0.0f,0.0f,1.0f);
    glTexCoord2f(tx1,ty1);
    glVertex2f(x,y);
    glTexCoord2f(tx2,ty1);
    glVertex2f(x+w,y);
    glTexCoord2f(tx2,ty2);
    glVertex2f(x+w,y+h);
    glTexCoord2f(tx1,ty2);
    glVertex2f(x,y+h);
    glEnd();
    glDisable(GL_TEXTURE_2D);

    return;
  }

  def_blit(dest,x,y,w,h,src,src_x,src_y,lgop);
}


/************************************************** Override stdbitmap */

g_error sdlgl_bitmap_get_groprender(hwrbitmap bmp, struct groprender **rend) {
  struct glbitmap *glb = (struct glbitmap *) bmp;
  
  /* Special case for the display */
  if (!bmp) {
    g_error e;

    if (gl_display_rend) {
      *rend = gl_display_rend;
      return success;
    }

    e = g_malloc((void **) &gl_display_rend,sizeof(struct groprender));
    errorcheck;
    memset(gl_display_rend,0,sizeof(struct groprender));
    gl_display_rend->lgop = PG_LGOP_NONE;
    gl_display_rend->output = NULL;
    gl_display_rend->hfont = defaultfont;
    gl_display_rend->clip.x2 = vid->lxres - 1;
    gl_display_rend->clip.y2 = vid->lyres - 1;
    gl_display_rend->orig_clip = gl_display_rend->clip;
    gl_display_rend->output_rect.w = vid->lxres;
    gl_display_rend->output_rect.h = vid->lyres;

    return success;
  }

  return def_bitmap_get_groprender(glb->sb,rend);
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

/************************************************** Input hooks for camera movement */

int sdlgl_key_event_hook(u32 *type, s16 *key, s16 *mods) {

  /* Entering/leaving modes */

  if ((*mods & PGMOD_CTRL) && (*mods & PGMOD_ALT) && *type==PG_TRIGGER_KEYDOWN)
    switch (*key) {
    case PGKEY_q:
      if (gl_camera_mode == SDLGL_CAMERAMODE_TRANSLATE)
	gl_camera_mode = SDLGL_CAMERAMODE_NONE;
      else
	gl_camera_mode = SDLGL_CAMERAMODE_TRANSLATE;
      return 1;
    case PGKEY_e:
      if (gl_camera_mode == SDLGL_CAMERAMODE_ROTATE)
	gl_camera_mode = SDLGL_CAMERAMODE_NONE;
      else
	gl_camera_mode = SDLGL_CAMERAMODE_ROTATE;
      return 1;
    case PGKEY_r:
      gl_camera_mode = SDLGL_CAMERAMODE_NONE;
      glLoadIdentity();
      gl_matrix_pixelcoord();
      return 1;
    case PGKEY_f:
      gl_showfps = !gl_showfps;
      return 1;
    case PGKEY_g:
      gl_grid = !gl_grid;
      return 1;
    default:
      return 0;
    }
     
  /* In a camera mode? */
 
  if (gl_camera_mode != SDLGL_CAMERAMODE_NONE) {
    /* Keep track of pressed keys */

    if (*type == PG_TRIGGER_KEYDOWN)
      gl_pressed_keys[*key] = 1;
    if (*type == PG_TRIGGER_KEYUP)
      gl_pressed_keys[*key] = 0;

    /* A couple keys should exit camera mode... */
    switch (*key) {
    case PGKEY_ESCAPE:
    case PGKEY_SPACE:
    case PGKEY_RETURN:
      gl_camera_mode = SDLGL_CAMERAMODE_NONE;
    }

    /* Trap events */
    return 1;
  }

  /* Pass the event */
  return 0;
}

int sdlgl_pointing_event_hook(u32 *type, s16 *x, s16 *y, s16 *btn) {
  int dx,dy,dz=0;
  s16 cursorx,cursory;

  /* Just pass the event if we're not in a camera mode */
  if (gl_camera_mode == SDLGL_CAMERAMODE_NONE)
    return 0;

  /* Get the physical position of PicoGUI's cursor */
  cursorx = cursor->x;
  cursory = cursor->y;
  VID(coord_physicalize)(&cursorx,&cursory);
  
  /* get the movement since last time and warp the mouse back */
  dx = *x - cursorx;
  dy = *y - cursory;
  SDL_WarpMouse(cursorx,cursory);

  /* Translate the mouse wheel into Z motion */
  if (*type == PG_TRIGGER_DOWN && (*btn & 8))
    dz = 20;
  if (*type == PG_TRIGGER_DOWN && (*btn & 16))
    dz = -20;

  switch (gl_camera_mode) {

  case SDLGL_CAMERAMODE_TRANSLATE:
    glTranslatef(dx,dy,dz);
    break;

  case SDLGL_CAMERAMODE_ROTATE:
    glRotatef(dy/10.0,1,0,0);
    glRotatef(dx/10.0,0,1,0);
    glRotatef(dz/10.0,0,0,1);
    break;

  }
  
  /* Absorb the event */
  return 1;
}

void gl_process_camera_keys(void) {
  /* Process camera movement keys */
  switch (gl_camera_mode) {
    
  case SDLGL_CAMERAMODE_TRANSLATE:
    if (gl_pressed_keys[PGKEY_w])
      glTranslatef(0.0,0.0,5.0);
    if (gl_pressed_keys[PGKEY_s])
      glTranslatef(0.0,0.0,-5.0);
    if (gl_pressed_keys[PGKEY_UP])
      glTranslatef(0.0,5.0,0.0);
    if (gl_pressed_keys[PGKEY_DOWN])
      glTranslatef(0.0,-5.0,0.0);
    if (gl_pressed_keys[PGKEY_LEFT])
      glTranslatef(5.0,0.0,0.0);
    if (gl_pressed_keys[PGKEY_RIGHT])
      glTranslatef(-5.0,0.0,0.0);
    break;

  case SDLGL_CAMERAMODE_ROTATE:
    if (gl_pressed_keys[PGKEY_w])
      glRotatef(0.4,0.0,0.0,1.0);
    if (gl_pressed_keys[PGKEY_s])
      glRotatef(-0.4,0.0,0.0,1.0);
    if (gl_pressed_keys[PGKEY_UP])
      glRotatef(0.4,1.0,0.0,0.0);
    if (gl_pressed_keys[PGKEY_DOWN])
      glRotatef(-0.4,1.0,0.0,0.0);
    if (gl_pressed_keys[PGKEY_LEFT])
      glRotatef(0.4,0.0,1.0,0.0);
    if (gl_pressed_keys[PGKEY_RIGHT])
      glRotatef(-0.4,0.0,1.0,0.0);
    break;
    
  }
}

/************************************************** Fonts using textures and SDL_ttf */

/* Convert a TrueType font to a series of textures and store the metadata
 * in picogui's fontstyle list.
 */
g_error gl_load_font(const char *file) {
  TTF_Font *ttf;
  struct fontstyle_node *fsn;
  g_error e;
  char smallname[40];
  char *p;

  ttf = TTF_OpenFont(file, get_param_int("video-sdlgl","font_resolution",32));
  /* Don't complain about font loading errors, it's not that important */
  if (!ttf) return;

  /* Allocate a picogui fontstyle node */
  e = g_malloc((void**)&fsn, sizeof(struct fontstyle_node));
  errorcheck;
  memset(fsn,0,sizeof(struct fontstyle_node));

  /* Make a small name for the font, using the filename */
  p = strrchr(file,'/');
  if (!p) p = strrchr(file,'\\');
  if (!p) p = (char*) file;
  strncpy(smallname,p,sizeof(smallname));
  smallname[sizeof(smallname)-1] = 0;
  p = strrchr(smallname,'.');
  if (p) *p = 0;
  e = g_malloc((void**)&fsn->name, strlen(smallname)+1);
  errorcheck;
  strcpy((char*) fsn->name,smallname);

  fsn->size = TTF_FontHeight(ttf);

  /* Build all the font representations */

  e = gl_load_font_style(ttf,&fsn->normal,TTF_STYLE_NORMAL);
  errorcheck;
  e = gl_load_font_style(ttf,&fsn->bold,TTF_STYLE_BOLD);
  errorcheck;
  e = gl_load_font_style(ttf,&fsn->italic,TTF_STYLE_ITALIC);
  errorcheck;
  e = gl_load_font_style(ttf,&fsn->bolditalic,TTF_STYLE_BOLD | TTF_STYLE_ITALIC);
  errorcheck;

  /* Link into picogui's font list */
  fsn->next = fontstyles;
  fontstyles = fsn;

  TTF_CloseFont(ttf);
  return success;
}

/* Load all the glyphs from one font style into a texture, allocating a new struct font
 *
 * FIXME: This just loads glyphs 0 through 255. This breaks Unicode!
 *         We need a way to determine which glyphs actually exist.
 */
g_error gl_load_font_style(TTF_Font *ttf, struct font **ppf, int style) {
  g_error e;
  struct font *f;
  Uint16 ch;
  struct gl_glyph *glg;
  struct fontglyph *fg;
  int minx, maxx, miny, maxy, advance;
  SDL_Surface *surf;
  static GLuint texture = 0;
  static int tx = 0,ty = 0, tline = 0;
  static SDL_Color white = {0xFF,0xFF,0xFF,0};
  static SDL_Color black = {0x00,0x00,0x00,0};
  u32 i;
  u8 *p;

  TTF_SetFontStyle(ttf, style);

  /* Set up a new picogui font structure */
  e = g_malloc((void**)ppf, sizeof(struct font));
  errorcheck;
  f = *ppf;
  memset(f,0,sizeof(struct font));

  f->numglyphs = 256;
  f->ascent = TTF_FontAscent(ttf);
  f->descent = -TTF_FontDescent(ttf);
  
  /* Allocate the glyph bitmap array and the glyph array */
  e = g_malloc((void**)&f->bitmaps, sizeof(struct gl_glyph) * f->numglyphs);
  errorcheck;
  glg = (struct gl_glyph*) f->bitmaps;
  e = g_malloc((void**)&f->glyphs, sizeof(struct fontglyph) * f->numglyphs);
  errorcheck;
  fg = (struct fontglyph*) f->glyphs;

  /* FIXME: This won't work for Unicode, and it's inefficient anyways.
   * we need a way to figure out what glyphs actually exist in the font.
   */
  for (ch=0;ch<256;ch++) {
    TTF_GlyphMetrics(ttf, ch, &minx, &maxx, &miny, &maxy, &advance);
    surf = TTF_RenderGlyph_Shaded(ttf, ch, white,black);
    
    /* Not enough space on this line? */
    if (tx+maxx-minx+1 > GL_FONT_TEX_SIZE) {
      tx = 0;
      ty += tline+1;
    }

    /* Texture nonexistant or full? Make a new one */
    if (!texture || ty+maxy-miny+1 > GL_FONT_TEX_SIZE) {
      /* A chunk of zeroes we'll use to make sure the unused portions of the font
       * bitmap are cleared. This will avoid filtering artifacts at the edges of fonts.
       */
      char *p;
      e = g_malloc((void**)&p, GL_FONT_TEX_SIZE * GL_FONT_TEX_SIZE);
      errorcheck;
      memset(p,0,GL_FONT_TEX_SIZE * GL_FONT_TEX_SIZE);

      glGenTextures(1,&texture);
      glBindTexture(GL_TEXTURE_2D, texture);
      glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,gl_texture_filtering);
      glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,gl_texture_filtering);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_INTENSITY4, GL_FONT_TEX_SIZE, GL_FONT_TEX_SIZE, 0, 
		   GL_LUMINANCE, GL_UNSIGNED_BYTE, p);
      tx = ty = tline = 0;
      g_free(p);
    }

    /* Scale the glyph bitmap into the full range 0-255 */
    p = (u8*) surf->pixels;
    for (i=surf->pitch*surf->h;i;i--,p++)
      *p = (*p)*255/4;
      
    /* Copy our new glyph at (tx,ty) */
    glTexSubImage2D(GL_TEXTURE_2D, 0, tx,ty, surf->w, surf->h,
		    GL_LUMINANCE, GL_UNSIGNED_BYTE, surf->pixels);
 
    /* Save the texture data in the 'bitmaps' table */
    glg[ch].texture = texture;
    glg[ch].tx1 = (float)tx / (float)GL_FONT_TEX_SIZE;
    glg[ch].ty1 = (float)ty / (float)GL_FONT_TEX_SIZE;
    glg[ch].tx2 = (float)(tx+surf->w) / (float)GL_FONT_TEX_SIZE;
    glg[ch].ty2 = (float)(ty+surf->h) / (float)GL_FONT_TEX_SIZE;

    /* Fill in the normal picogui glyph data */
    fg[ch].encoding = ch;
    fg[ch].bitmap = ch * sizeof(struct gl_glyph);
    fg[ch].dwidth = advance;
    fg[ch].w = surf->w;
    fg[ch].h = surf->h;
    fg[ch].x = minx;
    fg[ch].y = 0;

    if (surf->w > f->w)
      f->w = surf->w;
    if (surf->h > f->h+f->descent)
      f->h = surf->h - f->descent;

    /* Increment cursor in the texture */
    tx += surf->w+1;
    if (tline < surf->h)
      tline = surf->h;
    
    SDL_FreeSurface(surf);
  }  

  return success;
}

void sdlgl_font_newdesc(struct fontdesc *fd, const u8 *name, int size, int flags) {
  /* Let them have any size, dont constrain to the size in the fontstyle.
   * Store the size in "extra"
   */
  fd->extra = (void*) size;
}

void sdlgl_font_outtext_hook(hwrbitmap *dest, struct fontdesc **fd,
			     s16 *x,s16 *y,hwrcolor *col,const u8 **txt,
			     struct quad **clip, s16 *lgop, s16 *angle) {
  int size = (int) (*fd)->extra;  
  float scale = (float)size / (*fd)->font->h;
  int ch;
  struct fontglyph const *g;
  struct gl_glyph *glg;

  if (GL_LINEAR32(dest)) {
    /* FIXME: Can't render to offscreen bitmaps yet */
    return;
  }

  /* FIXME: No lgops for text yet, but i don't think anything actually
   *        uses those yet.
   */

  /* Set up blending and texturing */
  gl_lgop(PG_LGOP_ALPHA);
  glEnable(GL_TEXTURE_2D);
  gl_color(*col);

  /* Use OpenGL's matrix for translation, scaling, and rotation 
   * (So much easier than picogui's usual method :)
   */
  glPushMatrix();
  glTranslatef(*x,*y,0);
  glScalef(scale,scale,scale);
  glRotatef(*angle,0,0,1);

  while ((ch = (*fd)->decoder(txt))) {
    if (ch=='\n')
      glTranslatef(0,(*fd)->font->h + (*fd)->interline_space,0);
    else if (ch!='\r') {
      if ((*fd)->passwdc > 0)
	ch = (*fd)->passwdc;

      g = vid->font_getglyph(*fd,ch);
      glg = (struct gl_glyph*)(((u8*)(*fd)->font->bitmaps)+g->bitmap);
      
      glBindTexture(GL_TEXTURE_2D, glg->texture);
      glBegin(GL_QUADS);
      glNormal3f(0.0f,0.0f,1.0f);
      glTexCoord2f(glg->tx1,glg->ty1);
      glVertex2f(0,0);
      glTexCoord2f(glg->tx2,glg->ty1);
      glVertex2f(g->w,0);
      glTexCoord2f(glg->tx2,glg->ty2);
      glVertex2f(g->w,g->h);
      glTexCoord2f(glg->tx1,glg->ty2);
      glVertex2f(0,g->h);
      glEnd();

      glTranslatef(g->dwidth,0,0);
    }
  }

  /* Clean up */
  glPopMatrix();
  glDisable(GL_TEXTURE_2D);
  gl_lgop(PG_LGOP_NONE);

  /* Prevent normal outtext rendering */
  *txt = "";
  *lgop = PG_LGOP_NONE;
}
 
void sdlgl_font_sizetext_hook(struct fontdesc *fd, s16 *w, s16 *h, const u8 *txt) {
  int size = (int) fd->extra;

  /* Most of the sizetext should still be fine for us, but we need to scale
   * the final result by the real requested font size.
   */
  *w = (*w) * size / fd->font->h;
}

/************************************************** Initialization */

g_error sdlgl_init(void) {
  const char *s;
  int i;
  g_error e;

  /* Default mode: 640x480 */
  if (!vid->xres) vid->xres = 640;
  if (!vid->yres) vid->yres = 480;

  /* Start up the SDL video subsystem thingy */
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
  if (get_param_int("video-sdlgl","continuous",0)) {
    e = load_inlib(&gl_continuous_regfunc,&gl_continuous);
    errorcheck;
  }

  s = get_param_str("video-sdlgl","texture_filtering","linear");
  if (!strcmp(s,"linear")) {
    gl_texture_filtering = GL_LINEAR;
  }
  else {
    gl_texture_filtering = GL_NEAREST;
  }

  sscanf(get_param_str("video-sdlgl","fps_interval","0.25"),"%f",&gl_fps_interval);
  
  /* Load a main input driver */
  return load_inlib(&sdlinput_regfunc,&inlib_main);
}

g_error sdlgl_setmode(s16 xres,s16 yres,s16 bpp,u32 flags) {
  unsigned long sdlflags = SDL_RESIZABLE | SDL_OPENGL;
  char str[80];
  float a,x,y,z;
  g_error e;
   
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

  /********** OpenGL setup */

  /* Clear */
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  glClearDepth(1.0);

  /* Options */
  //  glDepthFunc(GL_LEQUAL);
  //  glEnable(GL_DEPTH_TEST);
  glShadeModel(GL_SMOOTH);

  /* Set up camera so that we have 0,0 at the top-left,
   * xres,yres at the bottom-right, but we want perspective
   */
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(GL_FOV,1,GL_MINDEPTH,GL_MAXDEPTH);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  /* Apply the transformations that should be done while
   * the origin is in the center and each edge is one unit from the origin.
   */
  
  sscanf(get_param_str("video-sdlgl","unit_rotate","0 0 0 0"), "%f %f %f %f", &a,&x,&y,&z);
  glRotatef(a,x,y,z);

  sscanf(get_param_str("video-sdlgl","unit_scale","1 1 1"), "%f %f %f", &x,&y,&z);
  glScalef(x,y,z);

  sscanf(get_param_str("video-sdlgl","unit_translate","0 0 0"), "%f %f %f", &x,&y,&z);
  glTranslatef(x,y,z);

  /* Now convert to pixel coordinates */
  gl_matrix_pixelcoord();

  /* Apply the transformations that should be done in pixels
   */
  
  sscanf(get_param_str("video-sdlgl","pixel_rotate","0 0 0 0"), "%f %f %f %f", &a,&x,&y,&z);
  glRotatef(a,x,y,z);

  sscanf(get_param_str("video-sdlgl","pixel_scale","1 1 1"), "%f %f %f", &x,&y,&z);
  glScalef(x,y,z);

  sscanf(get_param_str("video-sdlgl","pixel_translate","0 0 0"), "%f %f %f", &x,&y,&z);
  glTranslatef(x,y,z);


  /* Remove normal picogui fonts */
  gl_old_fonts = fontstyles;
  fontstyles = NULL;

  /* Load Truetype fonts */
  gl_load_font("/usr/share/fonts/truetype/openoffice/timmonsr.ttf");  

  e = findfont(&gl_osd_font,-1,NULL,get_param_int("video-sdlgl","osd_fontsize",20),0);
  errorcheck;


  return success; 
}
   
void sdlgl_close(void) {
  if (gl_display_rend) {
    g_free(gl_display_rend);
    gl_display_rend = NULL;
  }

  unload_inlib(inlib_main);   /* Take out our input driver */
  if (gl_continuous)
    unload_inlib(gl_continuous);

  if (gl_osd_font)
    handle_free(gl_osd_font,-1);

  /* Put back normal fonts */
  fontstyles = gl_old_fonts;

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

/************************************************** Registration */

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
  v->font_sizetext_hook = &sdlgl_font_sizetext_hook;
  v->font_newdesc = &sdlgl_font_newdesc;
  v->font_outtext_hook = &sdlgl_font_outtext_hook;

  return success;
}

/* The End */









