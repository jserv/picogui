/* $Id: sdlgl.h,v 1.18 2002/09/19 21:50:07 micahjd Exp $
 *
 * sdlgl.h - OpenGL driver for picogui, using SDL for portability
 *           This file holds definitions shared between components of
 *           the sdlgl driver, and it should NOT be included in other files.
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

#ifndef _H_SDLGL
#define _H_SDLGL

/************************************************** Headers */

#include <pgserver/video.h>       /* Always needed for a video driver! */
#include <pgserver/render.h>      /* For data types like 'quad' */
#include <pgserver/input.h>       /* For loading our corresponding input lib */
#include <pgserver/configfile.h>  /* For loading our configuration */
#include <pgserver/appmgr.h>      /* Default font */
#include <pgserver/font.h>        /* font rendering */

#ifdef DEBUG_VIDEODRIVER
#define DEBUG_FILE
#endif
#include <pgserver/debug.h>

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
  int volatilesb;           /* Indicates that the stdbitmap's contents may be changed without notice */
  u32 last_update_time;     /* Timer for the hack used in gl_make_texture */

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
 * by storing a pre-tiled copy of the bitmap. The GL_TILESIZE_IDEAL
 * should be at least twice GL_TILESIZE_MIN or pgserver could get stuck
 * in an infinite loop tiling bitmaps!
 */
#define GL_TILESIZE_MIN   128   /* If a texture dimension is less than this, pretile it */
#define GL_TILESIZE_IDEAL 256   /* Create pretiled textures at this size. Should be a power of 2! */

/* Texture and coordinates for one glyph. A table of these is in the font's
 * "bitmaps" array.
 */
struct gl_glyph {
  GLuint texture;
  float tx1,ty1,tx2,ty2;  /* Texture coordinates */
};

/* Power of two of our font texture's size */
#define GL_FONT_TEX_POWER 9

/* Size of the textures to use for font conversion, in pixels. MUST be a power of 2 */
#define GL_FONT_TEX_SIZE (1<<(GL_FONT_TEX_POWER))

/* There must be sufficient spacing between characters so that even in the mipmapped
 * font textures, there is no bleeding of colors between characters. This should be
 * one greater than the power of two used in GL_FONT_TEX_SIZE */
#define GL_FONT_SPACING ((GL_FONT_TEX_POWER)+1)

/* Macro to determine when to redirect drawing to linear32 */
#define GL_LINEAR32(dest) ((dest) && gl_invalidate_texture(dest))
#define GL_LINEAR32_SRC(src) (src)

/* Retrieve the associated stdbitmap for linear32 to operate on */
#define STDB(dest) (((struct glbitmap*)(dest))->sb)

/* Camera modes that let the user move the camera around */
#define SDLGL_CAMERAMODE_NONE      0
#define SDLGL_CAMERAMODE_TRANSLATE 1
#define SDLGL_CAMERAMODE_ROTATE  2

/* This is a temporary structure used during font loading. It must
 * be initialized and shut down properly. */
struct gl_fontload {
  GLuint texture;
  u8 *pixels;
  int tx,ty,tline;
};

/* Since calls to get_param_* are splattered out everywhere, define the
 * configfile section here.
 */
#define GL_SECTION "video-sdlgl"

/* This is a macro you can stick in for debugging to see what
 * the last OpenGL error was.
 */
#ifdef DEBUG_FILE
#define gl_errorcheck printf( "%s @ %d: %s\n", __FUNCTION__, __LINE__,gluErrorString(glGetError()))
#else
#define gl_errorcheck
#endif

/************************************************** Globals */

#ifdef CONFIG_SDLSKIN
extern s16 sdlfb_display_x;
extern s16 sdlfb_display_y;
extern u16 sdlfb_scale;
#endif

struct sdlgl_data {

  SDL_Surface *vidsurf;

  /* Type of texture filtering to use */
  GLint texture_filtering;
  
  /* Frames per second, calculated in gl_frame() */
  float fps;

  /* FPS calculation interval. Larger values are more accurate, smaller
   * values update the FPS value faster. Units are seconds.
   */
  float fps_interval;

  /* Input library optionally loaded to make this driver continously redraw */
  struct inlib *continuous_inlib;
  
  /* Groprender structure for the display */
  struct groprender *display_rend;

  /* Camera modes that let the user move the camera around */
  int camera_mode;

  /* Current camera position */
  struct {
    float tx,ty,tz;
    float rx,ry,rz;
  } camera;

  /* For the camera movement, keep track of which keys are pressed */
  u8 pressed_keys[PGKEY_MAX];
  
  /* Font for onscreen display */
  handle osd_font;
  
  /* More flags */
  int grid;
  int showfps;
  int antialias;
  int allow_update;
  int need_update;
  int continuous;
  int update_throttle;
  int wireframe;

  /* save the old font list so we can restore it on exit */
  struct fontstyle_node *old_fonts;
  
  /* Handle to sdlgl's input filter */
  handle h_infilter;
};

extern struct sdlgl_data gl_global;

/* This is the input filter sdlgl uses to handle camera control events */
extern struct infilter infilter_sdlgl;

/************************************************** Functions */

inline void gl_color(hwrcolor c);
inline float gl_dist_line_to_point(float point_x, float point_y, 
				   float line_px, float line_py,
				   float line_vx, float line_vy);
inline void gl_lgop(s16 lgop);
int gl_power2_round(int x);
void gl_frame(void);
void sdlgl_pixel(hwrbitmap dest,s16 x,s16 y,hwrcolor c,s16 lgop);
hwrcolor sdlgl_getpixel(hwrbitmap dest,s16 x,s16 y);
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
void gl_matrix_camera(void);
void gl_process_camera_keys(void);
void gl_render_grid(void);
g_error gl_load_font_style(struct gl_fontload *fl,TTF_Font *ttf, struct font **ppf, int style);
g_error gl_load_font(struct gl_fontload *fl,const char *file);
void sdlgl_font_newdesc(struct fontdesc *fd, const u8 *name, int size, int flags);
void sdlgl_font_outtext_hook(hwrbitmap *dest, struct fontdesc **fd,
			     s16 *x,s16 *y,hwrcolor *col,const struct pgstring **txt,
			     struct quad **clip, s16 *lgop, s16 *angle);
void sdlgl_font_sizetext_hook(struct fontdesc *fd, s16 *w, s16 *h, const struct pgstring *txt);
void gl_fontload_storetexture(struct gl_fontload *fl);
g_error gl_fontload_init(struct gl_fontload **fl);
void gl_fontload_finish(struct gl_fontload *fl); 
void sdlgl_blit(hwrbitmap dest, s16 x,s16 y,s16 w,s16 h, hwrbitmap src,
		s16 src_x, s16 src_y, s16 lgop);
float gl_get_key_scale(void);
void gl_fontstyle_free(struct fontstyle_node *fsn);
void gl_font_free(struct font *f);
void gl_showtexture(GLuint tex, int w, int h);
int gl_invalidate_texture(hwrbitmap bit);
int sdlgl_update_hook(void);
void sdlgl_sprite_show(struct sprite *spr);
void sdlgl_sprite_hide(struct sprite *spr);
void sdlgl_sprite_update(struct sprite *spr);
void sdlgl_sprite_protectarea(struct quad *in,struct sprite *from);
void gl_continuous_poll(void); 
int sdlgl_grop_render_presetup_hook(struct divnode **div, struct gropnode ***listp,
				    struct groprender *rend);
hwrcolor sdlgl_color_pgtohwr(pgcolor c);
pgcolor sdlgl_color_hwrtopg(pgcolor c);
void sdlgl_grop_handler(struct groprender *r, struct gropnode *n);
void gl_make_texture(struct glbitmap *glb);
void gl_set_wireframe(int on);
int sdlgl_grop_render_node_hook(struct divnode **div, struct gropnode ***listp,
				struct groprender *rend, struct gropnode *node);
int sdlgl_grop_render_postsetup_hook(struct divnode **div, struct gropnode ***listp,
				     struct groprender *rend);
void sdlgl_grop_render_end_hook(struct divnode **div, struct gropnode ***listp,
				struct groprender *rend);
g_error sdlgl_bitmap_getshm(hwrbitmap bmp, u32 uid, struct pgshmbitmap *shm);

#endif /* _H_SDLGL */

/* The End */









