/* $Id: sdlgl.c,v 1.5 2002/02/28 05:58:18 micahjd Exp $
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

#include <SDL/SDL.h>

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

/************************************************** Definitions */

/* Perspective values */
#define GL_FOV          45      /* Vertical field of view in degrees */
#define GL_MINDEPTH     0.1
#define GL_MAXDEPTH     100
#define GL_DEFAULTDEPTH 50
#define GL_SCALE        (tan(GL_FOV/360.0f*3.141592654)*GL_DEFAULTDEPTH)

SDL_Surface *sdlgl_vidsurf;

#ifdef CONFIG_SDLSKIN
extern s16 sdlfb_display_x;
extern s16 sdlfb_display_y;
extern u16 sdlfb_scale;
#endif

/* Type of texture filtering to use */
GLint gl_texture_filtering;

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

/* Macro to determine when to redirect drawing to linear32 */
#define GL_LINEAR32(dest) (dest)

/* Retrieve the associated stdbitmap for linear32 to operate on */
#define STDB(dest) (((struct glbitmap*)(dest))->sb)

/* Groprender structure for the display */
struct groprender *gl_display_rend = NULL;

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
 * FIXME: most of these haven't been tested
 */
inline void gl_lgop(s16 lgop) {
  switch (lgop) {

  case PG_LGOP_NULL:
    glDisable(GL_BLEND);
    glEnable(GL_LOGIC_OP);
    glLogicOp(GL_NOOP);
    break;

  case PG_LGOP_NONE:
    glDisable(GL_LOGIC_OP);
    glDisable(GL_BLEND);
    break;

  case PG_LGOP_OR:
    glDisable(GL_BLEND);
    glEnable(GL_LOGIC_OP);
    glLogicOp(GL_OR);
    break;

  case PG_LGOP_AND:
    glDisable(GL_BLEND);
    glEnable(GL_LOGIC_OP);
    glLogicOp(GL_AND);
    break;

  case PG_LGOP_XOR:
    glDisable(GL_BLEND);
    glEnable(GL_LOGIC_OP);
    glLogicOp(GL_XOR);
    break;

  case PG_LGOP_INVERT:
    glDisable(GL_BLEND);
    glEnable(GL_LOGIC_OP);
    glLogicOp(GL_INVERT);
    break;

  case PG_LGOP_INVERT_OR:
    glDisable(GL_BLEND);
    glEnable(GL_LOGIC_OP);
    glLogicOp(GL_OR_REVERSE);
    break;

  case PG_LGOP_INVERT_AND:
    glDisable(GL_BLEND);
    glEnable(GL_LOGIC_OP);
    glLogicOp(GL_AND_REVERSE);
    break;

  case PG_LGOP_INVERT_XOR:
    glDisable(GL_BLEND);
    glEnable(GL_LOGIC_OP);
    glLogicOp(GL_EQUIV);
    break;

  case PG_LGOP_ADD:
    glEnable(GL_BLEND);
    glDisable(GL_LOGIC_OP);
    glBlendFunc(GL_ONE,GL_ONE);
    break;

  case PG_LGOP_SUBTRACT:
    glDisable(GL_BLEND);
    glEnable(GL_LOGIC_OP);
    glLogicOp(GL_NOOP);
    break;

  case PG_LGOP_MULTIPLY:
    glEnable(GL_BLEND);
    glDisable(GL_LOGIC_OP);
    glBlendFunc(GL_ZERO,GL_SRC_COLOR);
    break;

  case PG_LGOP_STIPPLE:
    glDisable(GL_LOGIC_OP);
    glDisable(GL_BLEND);
    break;

  case PG_LGOP_ALPHA:
    glEnable(GL_BLEND);
    glDisable(GL_LOGIC_OP);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
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
  static u32 frames;

  for (p=dts->top;p;p=p->next)
    p->flags |= DIVTREE_ALL_REDRAW;
  update(NULL,1);

  frames++;
}

/************************************************** Basic primitives */

void sdlgl_pixel(hwrbitmap dest,s16 x,s16 y,hwrcolor c,s16 lgop) {
  if (GL_LINEAR32(dest)) {
    linear32_pixel(STDB(dest),x,y,c,lgop);
    return;
  }
  return;

  gl_lgop(lgop);
  glBegin(GL_POINTS);
  gl_color(c);
  glVertex2f(x,y);
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

  return 0;
  glReadPixels(x,y,1,1,GL_RED,GL_UNSIGNED_BYTE,&r);
  glReadPixels(x,y,1,1,GL_GREEN,GL_UNSIGNED_BYTE,&g);
  glReadPixels(x,y,1,1,GL_BLUE,GL_UNSIGNED_BYTE,&b);
  return mkcolor(r,g,b);
}

/* It's double-buffered */
void sdlgl_update(s16 x, s16 y, s16 w, s16 h) {
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

      /* Linear filtering */

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
    sdlgl_bitmap_free(glb->tile);
  if (glb->texture)
    glDeleteTextures(1,&glb->texture);
  g_free(glb);
}

/************************************************** Initialization */

g_error sdlgl_init(void) {
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

  /* Load a main input driver */
  return load_inlib(&sdlinput_regfunc,&inlib_main);
}

g_error sdlgl_setmode(s16 xres,s16 yres,s16 bpp,u32 flags) {
  unsigned long sdlflags = SDL_RESIZABLE | SDL_OPENGL;
  char str[80];
  float a,x,y,z;
  const char *s;
   
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

  /* Load params */
  
  s = get_param_str("video-sdlgl","texture_filtering","linear");
  if (!strcmp(s,"linear")) {
    gl_texture_filtering = GL_LINEAR;
  }
  else {
    gl_texture_filtering = GL_NEAREST;
  }

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

  glScalef(GL_SCALE,GL_SCALE,1.0f);
  glScalef(2.0f/xres,-2.0f/yres,1.0f);
  glTranslatef(-xres/2,-yres/2,-GL_DEFAULTDEPTH);

  /* Apply the transformations that should be done in pixels
   */
  
  sscanf(get_param_str("video-sdlgl","pixel_rotate","0 0 0 0"), "%f %f %f %f", &a,&x,&y,&z);
  glRotatef(a,x,y,z);

  sscanf(get_param_str("video-sdlgl","pixel_scale","1 1 1"), "%f %f %f", &x,&y,&z);
  glScalef(x,y,z);

  sscanf(get_param_str("video-sdlgl","pixel_translate","0 0 0"), "%f %f %f", &x,&y,&z);
  glTranslatef(x,y,z);
  
  return success; 
}
   
void sdlgl_close(void) {
  if (gl_display_rend) {
    g_free(gl_display_rend);
    gl_display_rend = NULL;
  }
  unload_inlib(inlib_main);   /* Take out our input driver */
  SDL_Quit();
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

  return success;
}

/* The End */









