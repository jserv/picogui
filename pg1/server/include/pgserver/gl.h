/* $Id$
 *
 * gl.h - Components defined and used by the OpenGL VBL for PicoGUI
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

#ifndef _H_PG_GL
#define _H_PG_GL

/************************************************** Headers */

#include <pgserver/video.h>       /* Always needed for a video driver! */
#include <pgserver/render.h>      /* For data types like 'quad' */
#include <pgserver/input.h>       /* For loading our corresponding input lib */
#include <pgserver/configfile.h>  /* For loading our configuration */

#ifdef DEBUG_VIDEODRIVER
#define DEBUG_FILE
#endif
#include <pgserver/debug.h>

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

/* Redefine PicoGUI's hwrbitmap
 * Note that it's very likely we'll want to use this as a texture, yet
 * OpenGL can't render directly into textures. So, we'll keep a stdbitmap
 * representation that we can draw on with linear32, and if necessary
 * we also have an OpenGL texture for quick blitting. The OpenGL texture
 * is only created when needed, and it is destroyed if the underlying
 * stdbitmap is modified.
 */
struct glbitmap {
  void *null;             /* This is null in a glbitmap, but non-null in a stdbitmap */
  
  struct stdbitmap *sb;
  int volatilesb;           /* Indicates that the stdbitmap's contents may be changed without notice */
  u32 last_update_time;     /* Timer for the hack used in gl_bind_texture */

  /* OpenGL texture, valid if texture is nonzero */
  GLuint texture;
  float tx1,ty1,tx2,ty2;  /* Texture coordinates */
  int tw,th;              /* Texture size (power of two) */
};

/* Like X11 and most other high-level graphics APIs, there is a relatively
 * large per-blit penalty. This becomes a big problem when drawing tiled
 * bitmaps. OpenGL natively handles tiling textures, but that doesn't help
 * us much since textures have to be a power of two in size. We solve this
 * by storing a pre-tiled copy of the bitmap. The GL_TILESIZE_IDEAL
 * should be at least twice GL_TILESIZE_MIN or pgserver could get stuck
 * in an infinite loop tiling bitmaps!
 */
#define GL_TILESIZE_MIN   128   /* If a texture dimension is less than this, pretile it */
#define GL_TILESIZE_IDEAL 256   /* Create pretiled textures at this size. Should be a power of 2! */

/* Macro to determine when to redirect drawing to linear32 */
#define GL_LINEAR32(dest) (dest && gl_invalidate_texture(dest))
#define GL_LINEAR32_SRC(src) (src)

/* Retrieve the associated stdbitmap for linear32 to operate on.
 * If this is already a stdbitmap, return that.
 */
#define STDB(dest) (((struct glbitmap*)dest)->null ? (dest) : (((struct glbitmap*)(dest))->sb))

/* Camera modes that let the user move the camera around */
#define GL_CAMERAMODE_NONE         0
#define GL_CAMERAMODE_TRANSLATE    1
#define GL_CAMERAMODE_ROTATE       2
#define GL_CAMERAMODE_FOLLOW_MOUSE 3

union gl_camera {
  double array[6];
  struct {
    double tx,ty,tz;
    double rx,ry,rz;
  } e;
};

/* Since calls to get_param_* are splattered out everywhere, define the
 * configfile section here.
 */
#define GL_SECTION "opengl"

/* This is a macro you can stick in for debugging to see what
 * the last OpenGL error was.
 */
#ifdef DEBUG_FILE
#define gl_errorcheck fprintf(stderr, "%s @ %d: %s\n", __FUNCTION__, __LINE__,gluErrorString(glGetError()))
#else
#define gl_errorcheck
#endif

/************************************************** Globals */

struct gl_data {
  /* Type of texture filtering to use */
  GLint texture_filtering;
  
  struct groprender *display_rend;

  /* Frames per second, calculated in gl_frame() */
  float fps;

  /* FPS calculation interval. Larger values are more accurate, smaller
   * values update the FPS value faster. Units are seconds.
   */
  float fps_interval;

  /* Input library optionally loaded to make this driver continously redraw */
  struct inlib *continuous_inlib;
  
  /* Camera modes that let the user move the camera around */
  int camera_mode;

  /* Current camera position */
  union gl_camera camera, smoothed_cam;

  /* For the camera movement, keep track of which keys are pressed */
  u8 pressed_keys[PGKEY_MAX];

  /* ID of the currently bound 2D texture */
  GLuint current_texture;
  
  /* Font for onscreen display */
  struct font_descriptor *osd_font;

  /* Y coordinate of the current OSD line */
  int osd_y;

  /* More flags */
  int grid;
  int showfps;
  int antialias;
  int allow_update;
  int need_update;
  int continuous;
  int update_throttle;
  int wireframe;
  int resetting;

  /* Handle to our input filter */
  handle h_infilter;
};

extern struct gl_data gl_global;

/* This is the input filter gl uses to handle camera control events */
extern struct infilter infilter_gl;


/************************************************** Functions */

inline void gl_color(hwrcolor c);
inline float gl_dist_line_to_point(float point_x, float point_y, 
				   float line_px, float line_py,
				   float line_vx, float line_vy);
inline void gl_lgop(s16 lgop);
int gl_power2_round(int x);
void gl_frame(void);
void gl_frame_setup(void);
void gl_frame_cleanup(void);
void gl_feedback(int x, int y, int w, int h, int lgop, int filter,
		 int source, int generate_mipmaps, int mipmap_level);
void gl_pixel(hwrbitmap dest,s16 x,s16 y,hwrcolor c,s16 lgop);
hwrcolor gl_getpixel(hwrbitmap dest,s16 x,s16 y);
void gl_rect(hwrbitmap dest,s16 x,s16 y,s16 w, s16 h, hwrcolor c,s16 lgop);
void gl_slab(hwrbitmap dest,s16 x,s16 y,s16 w, hwrcolor c,s16 lgop);
void gl_bar(hwrbitmap dest,s16 x,s16 y,s16 h, hwrcolor c,s16 lgop);
void gl_line(hwrbitmap dest,s16 x1,s16 y1,s16 x2,s16 y2,hwrcolor c,s16 lgop);
void gl_gradient(hwrbitmap dest,s16 x,s16 y,s16 w,s16 h,s16 angle,
		    pgcolor c1, pgcolor c2, s16 lgop);
g_error gl_init(void);
g_error gl_setmode(s16 xres,s16 yres,s16 bpp,u32 flags);
void gl_close(void);
g_error gl_regfunc(struct vidlib *v);
g_error gl_bitmap_get_groprender(hwrbitmap bmp, struct groprender **rend);
g_error gl_bitmap_getsize(hwrbitmap bmp,s16 *w,s16 *h);
g_error gl_bitmap_new(hwrbitmap *bmp,s16 w,s16 h,u16 bpp);
void gl_bitmap_free(hwrbitmap bmp);
void gl_continuous_init(int *n,fd_set *readfds,struct timeval *timeout);
g_error gl_continuous_regfunc(struct inlib *i);
void gl_osd_printf(const char *fmt, ...);
void gl_matrix_pixelcoord(void);
void gl_matrix_camera(void);
void gl_process_camera_keys(void);
void gl_process_camera_smoothing(void);
void gl_render_grid(void);
void gl_blit(hwrbitmap dest, s16 x,s16 y,s16 w,s16 h, hwrbitmap src,
		s16 src_x, s16 src_y, s16 lgop);
float gl_get_key_scale(void);
int gl_invalidate_texture(hwrbitmap bit);
int gl_update_hook(void);
void gl_sprite_show(struct sprite *spr);
void gl_sprite_hide(struct sprite *spr);
void gl_sprite_update(struct sprite *spr);
void gl_sprite_protectarea(struct pgquad *in,struct sprite *from);
void gl_continuous_poll(void); 
int gl_grop_render_presetup_hook(struct divnode **div, struct gropnode ***listp,
				    struct groprender *rend);
hwrcolor gl_color_pgtohwr(pgcolor c);
pgcolor gl_color_hwrtopg(pgcolor c);
void gl_grop_handler(struct groprender *r, struct gropnode *n);
void gl_bind_texture(struct glbitmap *glb);
void gl_set_wireframe(int on);
int gl_grop_render_node_hook(struct divnode **div, struct gropnode ***listp,
				struct groprender *rend, struct gropnode *node);
int gl_grop_render_postsetup_hook(struct divnode **div, struct gropnode ***listp,
				     struct groprender *rend);
void gl_grop_render_end_hook(struct divnode **div, struct gropnode ***listp,
				struct groprender *rend);
g_error gl_bitmap_getshm(hwrbitmap bmp, u32 uid, struct pgshmbitmap *shm);
void gl_multiblit(hwrbitmap dest, s16 x, s16 y, s16 w, s16 h,
		     hwrbitmap src, s16 sx, s16 sy, s16 sw, s16 sh, s16 xo, s16 yo, s16 lgop);
void gl_blur(hwrbitmap dest, s16 x, s16 y, s16 w, s16 h, s16 radius);

#endif /* _H_PG_GL */

/* The End */









