/* $Id: video.h,v 1.79 2002/03/03 18:26:42 micahjd Exp $
 *
 * video.h - Defines an API for writing PicoGUI video
 *           drivers
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

#ifndef __H_VIDEO
#define __H_VIDEO

#include <pgserver/g_error.h>
#include <pgserver/divtree.h>

struct fontdesc;
struct quad;
struct rect;
struct pair;
struct groprender;
struct gropnode;

/* Hardware-specific color value */
typedef u32 hwrcolor;

/* PicoGUI color (24-bit RGB)
   Usually converted to a hwrcolor at the first opportunity
*/
typedef u32 pgcolor;
#define getalpha(pgc)     (((pgc)>>24)&0x7F)
#define getred(pgc)       (((pgc)>>16)&0xFF)
#define getgreen(pgc)     (((pgc)>>8)&0xFF)
#define getblue(pgc)      ((pgc)&0xFF)
#define mkcolor(r,g,b)    (((r)<<16)|((g)<<8)|(b))
#define mkcolora(a,r,g,b) (((a)<<24)|((r)<<16)|((g)<<8)|(b)|PGCF_ALPHA)

/* Can be a hardware-specific bitmap, but usually is
 * a stdbitmap pointer. This is driver dependant.
 * It really doesn't matter what type of pointer
 * this is defined as, but it affects the compiler
 * warnings.
 */
typedef struct stdbitmap *hwrbitmap;

/* The hwrbitmap used in the default implementation
   (should be sufficient for most drivers)
*/
struct stdbitmap {
  u8  *bits;                 /* actual format depends on bpp */
  struct groprender *rend;   /* State for offscreen rendering */
  s16 w,h;
  u16 pitch;                 /* Spacing between lines, in bytes */
  u16 bpp;                   /* Bits per pixel of bitmap */

  /* Should 'bits' be freed also when bitmap is freed? */
  u16 freebits;    
};  /* NOTE: Allocating freebits as u16 is overkill, but this struct
     * must be word-aligned! */

/* A group of functions to deal with one bitmap format */
struct bitformat {
  u8 name[4];     /* fourcc for the format name */

  bool (*detect)(const u8 *data, u32 datalen);
  g_error (*load)(hwrbitmap *bmp, const u8 *data, u32 datalen);
  g_error (*save)(hwrbitmap bmp, u8 **data, u32 *datalen);
};
   
/* A sprite node, overlaid on the actual picture */
struct sprite {
  hwrbitmap *bitmap,*mask,backbuffer;
  s16 x,y;   /* Current coordinates */
  s16 ox,oy; /* Coordinates last time it was drawn */
  s16 w,h;   /* Dimensions of all buffers */
  s16 ow,oh; /* The blit last time, with clipping */
  struct divnode *clip_to;
  struct sprite *next;
  int lgop;  /* lgop to draw bitmap with, if there's no mask */
  unsigned int onscreen : 1;   /* Displayed on screen now */
  unsigned int visible  : 1;   /* Displayed under normal circumstances */
};
/* List of sprites to overlay */
extern struct sprite *spritelist;

/* This structure contains a pointer to each graphics function
   in use, forming a definition for a driver. Initially, all functions
   point to default implementations. It is only necessary for a driver
   to implement a few functions, but it can optionally implement others
   if they can be accelerated
*/

struct vidlib {

  /***************** Initializing and video modes */

  /* Required
   *   initializes graphics device. Setmode will be called immediately
   *   after this to initialize the default mode.
   */
  g_error (*init)(void);

  /* Recommended
   *   Changes the video mode immediately after initialization or at the
   *   client's request. The driver should pick the closest mode, and
   *   if it doesn't support mode switching simply ignore this. The client
   *   needs to check the video mode afterwards, as this is only a _request_
   *   for a particular mode.
   *
   * Default implementation: does nothing 
   */
  g_error (*setmode)(s16 xres,s16 yres,s16 bpp,u32 flags);

  /* Optional
   *   This is called after mode setting is complete, including all wrappers
   *   loaded. Here the driver can do any special munging that this mode
   *   needs.
   */
  g_error (*entermode)(void);
   
  /* Optional
   *   This is called any time the current mode is exited, whether by
   *   changing modes or changing drivers, but before the mode is actually
   *   switched. If the driver rearranged bitmap data in setmode, put it
   *   back to normal here.
   */
  g_error (*exitmode)(void);
   
  /* Recommended
   *   free memory, close device, etc. 
   * 
   * Default implementation: does nothing
   */
  void (*close)(void);

  /* Optional
   *   Converts physical coordinates (from an input device)
   *   into logical coordinates
   * 
   * Default implementation: does nothing
   */
  void (*coord_logicalize)(s16 *x,s16 *y);

  /* Optional
   *   Converts logical coordinates back to physical
   *
   * Default implementation: does nothing
   */
  void (*coord_physicalize)(s16 *x,s16 *y);

  /* Optional
   *   For video wrappers, rotate keypads along with the display
   *
   * Default implementation: does nothing
   */
  void (*coord_keyrotate)(s16 *k);

  /* Reccomended (without double-buffer, it looks really dumb)
   *   Update changes to the screen, if device is double-buffered or
   *   uses page flipping
   *
   * Default implementation: does nothing (assumes changes are immediate)
   */
  void (*update)(s16 x,s16 y,s16 w,s16 h);
   
  /* Current mode (only used in driver)
   *
   * The default bitmap functions should handle 1,2,4,8,16,24,32 bpp ok
   */
  s16 xres,yres,bpp;
  u32 flags;

  /* Logical screen size, read-only outside of driver */
  s16 lxres,lyres;
   
  /* fb_mem and fb_bpl are no longer here. Use display->bits and
   * display->pitch, also accessable with the macros FB_MEM and FB_BPL */
 
  /* This bitmap is passed to primitives to indicate rendering to the display.
   * When possible, it can have an actual pointer to video memory so the
   * display is no different from any other bitmap. If it must be a special
   * case, set this to NULL */
  hwrbitmap display;
   
  /* Optionally process driver messages */
  void (*message)(u32 message, u32 param, u32 *ret);

  /***************** Hooks */

  /* These hooks let picogui video drivers take over functionality
   * normally handled by other parts of picogui.
   */

  /* Optional
   *   Called after a new fontdesc is created. The video driver
   *   may choose to modify the font or cache things or something.
   *   The name,size, and flags passed to findfont are also included.
   * 
   * Default implementation: none
   */
  void (*font_newdesc)(struct fontdesc *fd, const u8 *name, int size, int flags);
  /* Utility to do a binary search for a font glyph */
  
  /* Optional
   *   Called to look up the glyph associated with a character encoding.
   *   The video driver may modify this to implement fake font logic,
   *   for an ASCII display or something other strange device.
   *
   * Default implementation: binary search of the font's glyph table
   */
  struct fontglyph const *(*font_getglyph)(struct fontdesc *fd, int ch);
   
  /* Optional
   *   Called after sizetext has run, giving the driver an opportunity
   *   to modify the sizes reported by it
   *
   * Default implementation: none
   */
  void (*font_sizetext_hook)(struct fontdesc *fd, s16 *w, s16 *h, const u8 *txt);

  /* Optional
   *   Called before outtext, giving the driver an opportunity to modify
   *   or observe the data sent to it
   *
   * Default implementation: none
   */
  void (*font_outtext_hook)(hwrbitmap *dest, struct fontdesc **fd,
			    s16 *x,s16 *y,hwrcolor *col,const u8 **txt,
			    struct quad **clip, s16 *lgop, s16 *angle);

  /* Optional
   *   Called before outchar, giving the driver an opportunity to modify
   *   or observe the data sent to it
   *
   * Default implementation: none
   */
  void (*font_outchar_hook)(hwrbitmap *dest, struct fontdesc **fd,
			    s16 *x,s16 *y,hwrcolor *col,int *c,
			    struct quad **clip, s16 *lgop, s16 *angle);

  /* Optional
   *   Called for every incoming key event. Return 1 to prevent normal
   *   key processing.
   */
  int (*key_event_hook)(u32 *type, s16 *key, s16 *mods);

  /* Optional
   *   Called for every incoming pointing event. Return 1 to prevent normal
   *   processing.
   */
  int (*pointing_event_hook)(u32 *type, s16 *x, s16 *y, s16 *btn);

  /* Optional
   *   Called at the beginning of grop_render, before anything has been set up.
   *   Abort setup if this returns nonzero
   */
  int (*grop_render_presetup_hook)(struct divnode **div, struct gropnode ***listp,
				   struct groprender *rend);

  /* Optional
   *   Called at the beginning of grop_render, but after setup
   *   Abort grop_render if this returns nonzero
   */
  int (*grop_render_postsetup_hook)(struct divnode **div, struct gropnode ***listp,
				    struct groprender *rend);

  /* Optional
   *   Called at the end of grop_render
   */
  void (*grop_render_end_hook)(struct divnode **div, struct gropnode ***listp,
			       struct groprender *rend);

  /* Optional
   *   Called before gropnode transformation but after flag testing.
   *   Return nonzero to skip the normal gropnode transformation and clipping.
   */
  int (*grop_render_node_hook)(struct divnode **div, struct gropnode ***listp,
			       struct groprender *rend, struct gropnode **node);

  /* Optional
   *   Called for any picogui update(). Return nonzero to abort the update
   */
  int (*update_hook)(void);

  /***************** Colors */

  /* Optional
   *   Convert a color to/from the driver's native format
   *
   * Default implementation:
   *    < 8bpp:  Assumes grayscale
   *      8bpp:  2-3-3 color
   *     16bpp:  5-6-5 color
   *  >= 24bpp:  No change
   */
  hwrcolor (*color_pgtohwr)(pgcolor c);
  pgcolor (*color_hwrtopg)(hwrcolor c);

  /***************** Primitives */

  /* Required
   *   Draw a pixel to the screen, in the hardware's color format
   */
  void (*pixel)(hwrbitmap dest, s16 x, s16 y, hwrcolor c, s16 lgop);

  /* Required
   *   Get a pixel, in hwrcolor format
   */
  hwrcolor (*getpixel)(hwrbitmap src, s16 x, s16 y);

  /* Very Recommended
   *   draws a continuous horizontal run of pixels
   *
   * Default implementation: draws a 1 pixel high rectangle
   */
  void (*slab)(hwrbitmap dest, s16 x, s16 y, s16 w, hwrcolor c, s16 lgop);

  /* Optional
   *   draws a vertical bar of pixels (a sideways slab)
   *
   * Default implementation: draws a 1 pixel wide rectangle
   */
  void (*bar)(hwrbitmap dest, s16 x,s16 y,s16 h,hwrcolor c, s16 lgop);

  /* Optional
   *   draws an arbitrary line between 2 points
   *
   * Default implementation: bresenham algrorithm and
   *   many calls to pixel()
   */
  void (*line)(hwrbitmap dest, s16 x1,s16 y1,s16 x2,s16 y2,
	       hwrcolor c, s16 lgop);

  /* Optional
   *   fills a rectangular area with a color
   *
   * Default implementation: Looped calls to slab()
   */
  void (*rect)(hwrbitmap dest, s16 x,s16 y,s16 w,s16 h,hwrcolor c, s16 lgop);

  /* Optional
   *   fills a rectangular area with a linear gradient
   *   of an arbitrary angle (in degrees), between two colors.
   *   If angle==0, c1 is at the left and c2 is at the
   *   right.  The gradient rotates clockwise as angle
   *   increases.
   *
   * Default implementation: interpolation algorithm, pixel()
   */
  void (*gradient)(hwrbitmap dest, s16 x,s16 y,s16 w,s16 h,s16 angle,
		   pgcolor c1, pgcolor c2, s16 lgop);
  
  /* Very Reccomended
   *   Blits a bitmap to screen, optionally using lgop.
   *   If w and/or h is bigger than the source bitmap, it
   *   should tile.
   * 
   * Default implementation: pixel!
   */
  void (*blit)(hwrbitmap dest, s16 x,s16 y,s16 w,s16 h, hwrbitmap src,
	       s16 src_x, s16 src_y, s16 lgop);

  /* Optional
   *   A version of blit() that blits bottom-up rather than top-down.
   *   This is needed for scrolling a region downwards.
   *
   * Default implementation: A call to blit() for each line
   */
  void (*scrollblit)(hwrbitmap dest, s16 x,s16 y,s16 w,s16 h, hwrbitmap src,
		     s16 src_x, s16 src_y, s16 lgop);

  /* Reccomended
   *   Blits a bitmap or a section of a bitmap repeatedly
   *   to cover an area. Used by many bitmap themes.
   *   The difference between blit and tileblit is that blit restarts tiles
   *   at the beginning of the bitmap, and tileblit restarts tiles at the
   *   beginning of the tiled section
   *
   * Default implementation: Many calls to blit()!
   */
  void (*tileblit)(hwrbitmap dest, s16 x, s16 y, s16 w, s16 h,
		   hwrbitmap src, s16 sx, s16 sy, s16 sw, s16 sh, s16 lgop);

  /* Reccomended
   *   Used for character data.  Blits 1bpp data from
   *   chardat to the screen, filling '1' bits with the
   *   color 'col'.  If lines > 0, every 'lines' lines
   *   the image is left-shifted one pixel, to simulate
   *   italics.
   * 
   *   Although in the future arbitrary rotation may be supported, currently
   *   'angle' must be a PG_DIR_* constant (measured in degrees)
   *
   * Default implementation: pixel(). Need I say more?
   */
  void (*charblit)(hwrbitmap dest, u8 *chardat, s16 x, s16 y, s16 w, s16 h,
		   s16 lines, s16 angle, hwrcolor c, struct quad *clip,
		   s16 lgop);
 
  void (*ellipse) (hwrbitmap dest, s16 x, s16 y, s16 w, s16 h, hwrcolor c, s16 lgop); 
  void (*fellipse) (hwrbitmap dest, s16 x, s16 y, s16 w, s16 h, hwrcolor c, s16 lgop); 
  void (*fpolygon) (hwrbitmap dest, s32* array, s16 xoff, s16 yoff , hwrcolor c, s16 lgop);
   
  /***************** Bitmaps */

  /* These functions all use the stdbitmap structure.
     If your driver needs a different bitmap format,
     implement all these functions. (also implement set/get pixel above)
     If not, you should be ok leaving these with the defaults.
  */

  /* Optional
   *   Allocates a new bitmap, and loads the
   *   formatted bitmap data into it
   *
   * Default implementation: transcribes pixels with vid->pixel()
   */

#ifdef CONFIG_FORMAT_XBM
  /* XBM 1-bit data (used internally) */
  g_error (*bitmap_loadxbm)(hwrbitmap *bmp,const u8 *data, s16 w, s16 h,
			    hwrcolor fg,hwrcolor bg);
#endif
   
  /* Load a bitmap, detecting the appropriate format */
  g_error (*bitmap_load)(hwrbitmap *bmp,const u8 *data,u32 datalen);

  /* Optional
   *   Rotates a bitmap by 90 degrees anticlockwise
   *
   * Default implementation: Assumes linear bitmap, has code for
   *                         all common bit depths
   */
  g_error (*bitmap_rotate90)(hwrbitmap *bmp);
   
  /* Optional
   *   Allocates an empty bitmap
   *
   * Default implementation: g_malloc, of course!
   */
  g_error (*bitmap_new)(hwrbitmap *bmp,
			s16 w,s16 h,u16 bpp);

  /* Optional
   *   Frees bitmap memory
   *
   * Default implementation: g_free...
   */
  void (*bitmap_free)(hwrbitmap bmp);

  /* Optional
   *   Gets size of a bitmap
   *
   * Default implementation: stdbitmap
   */
  g_error (*bitmap_getsize)(hwrbitmap bmp,s16 *w,s16 *h);

  /* Optional
   *   This is called for every bitmap when entering a new bpp or loading
   *   a new driver. Converts a bitmap from a linear array of 32-bit
   *   pgcolor values to the hwrcolors for this mode
   *
   * Default implementation: stdbitmap
   */
  g_error (*bitmap_modeconvert)(hwrbitmap *bmp);
   
  /* Optional
   *   The reverse of bitmap_modeconvert, this converts the bitmap from
   *    the hardware-specific format to a pgcolor array
   * 
   * Default implementation: stdbitmap
   */
  g_error (*bitmap_modeunconvert)(hwrbitmap *bmp);
   
  /* Optional
   *   Return the groprender structure associated with the bitmap,
   *   and if one does not yet exist, create it.
   *
   * Default implementation: stdbitmap
   */
  g_error (*bitmap_get_groprender)(hwrbitmap bmp, struct groprender **rend);

  /***************** Sprites */

  /* Optional
   *   Draws the bitmap
   *
   * Default implementation: uses blit, stores a backbuffer
   */
  void (*sprite_show)(struct sprite *spr);
  
  /* Optional
   *   Undraws the bitmap
   *
   * Default implementation: blits the backbuffer back onto the screen
   */
  void (*sprite_hide)(struct sprite *spr);

  /* Optional
   *   Repositions a sprite after coordinate change
   *
   * Default implementation: redraws sprite stack as necessary,
   *                         uses sprite_show and sprite_hide
   */
  void (*sprite_update)(struct sprite *spr);

  /* Optional
   *   Shows/hides all sprites in preparation for modifications
   * 
   * Default implementation: traverses list of sprites, calls show and hide
   */
  void (*sprite_hideall)(void);
  void (*sprite_showall)(void);

  /* Optional
   *   Removes any necessary sprites from a given area to protect it from
   *   screen updates. If applicable, only hides sprites starting with
   *   and above the specified sprite.
   * 
   * Default implementation: Calls sprite_hide for sprites in the area
   */
  void (*sprite_protectarea)(struct quad *in,struct sprite *from);
   
};

/* Currently in-use video driver */
extern struct vidlib *vid;

/* Optional wrapper around that driver that provides some transformation */
extern struct vidlib *vidwrap;

/* This macro is used to call a video function */
#define VID(f) (*vidwrap->f)

/* Trig (sin*256 from 0 to 90 degrees) */
extern unsigned char trigtab[];

/*
  Unloads previous driver, sets up a new driver and
  initializes it.  This is for changing the driver,
  and optionally changing the mode.  If you just
  want to change the mode use vid->setmode
*/
g_error load_vidlib(g_error (*regfunc)(struct vidlib *v),
		    s16 xres,s16 yres,s16 bpp,u32 flags);

/* List of installed video drivers */
struct vidinfo {
  const u8 *name;
  g_error (*regfunc)(struct vidlib *v);
};
extern struct vidinfo videodrivers[];

g_error (*find_videodriver(const u8 *name))(struct vidlib *v);

/* Set the video mode using the current driver. This is the implementation
 * of the setmode client request */
g_error video_setmode(u16 xres,u16 yres,u16 bpp,u16 flagmode,u32 flags);

/* Sprite helper functions */
g_error new_sprite(struct sprite **ps,s16 w,s16 h);
void free_sprite(struct sprite *s);

/* Sprite vars */
extern struct sprite *spritelist;

/* Helper functions for keeping an update region, used
   for double-buffering by the video drivers */
extern s16 upd_x;
extern s16 upd_y;
extern s16 upd_w;
extern s16 upd_h;
void add_updarea(s16 x,s16 y,s16 w,s16 h);
void realize_updareas(void);

/* Iterator functions to convert between pgcolor arrays and hwrcolor arrays */
g_error array_pgtohwr(u32 **array);
g_error array_hwrtopg(u32 **array);

/* Convert a array to a palette */
g_error array_palettize(handle h, int owner);

/* Generic functions from the default VBL. Drivers will usually choose to
 * only process the most commonly used options, to save code space. When
 * an unsupported LGOP or other parameter is used, the corresponding
 * defaultvbl function should be called
 */

void emulate_dos(void);
void def_update(s16 x, s16 y, s16 w, s16 h);
g_error def_setmode(s16 xres,s16 yres,s16 bpp,u32 flags);
hwrcolor def_color_pgtohwr(pgcolor c);
pgcolor def_color_hwrtopg(hwrcolor c);
void def_pixel(hwrbitmap dest, s16 x, s16 y, hwrcolor c, s16 lgop);
hwrcolor def_getpixel(hwrbitmap src, s16 x, s16 y);
void def_slab(hwrbitmap dest, s16 x, s16 y, s16 w, hwrcolor c, s16 lgop);
void def_bar(hwrbitmap dest, s16 x,s16 y,s16 h,hwrcolor c, s16 lgop);
void def_line(hwrbitmap dest, s16 x1,s16 y1,s16 x2,s16 y2,
	      hwrcolor c, s16 lgop);
void def_rect(hwrbitmap dest, s16 x,s16 y,s16 w,s16 h,hwrcolor c, s16 lgop);
void def_gradient(hwrbitmap dest, s16 x,s16 y,s16 w,s16 h,s16 angle,
		  pgcolor c1, pgcolor c2, s16 lgop);
void def_blit(hwrbitmap dest, s16 x,s16 y,s16 w,s16 h, hwrbitmap src,
	      s16 src_x, s16 src_y, s16 lgop);
void def_tileblit(hwrbitmap dest, s16 x, s16 y, s16 w, s16 h,
		  hwrbitmap src, s16 sx, s16 sy, s16 sw, s16 sh, s16 lgop);
void def_charblit(hwrbitmap dest, u8 *chardat, s16 x, s16 y, s16 w, s16 h,
		  s16 lines, s16 angle, hwrcolor c, struct quad *clip,
		  s16 lgop);
void def_sprite_protectarea(struct quad *in,struct sprite *from);
g_error def_bitmap_loadxbm(hwrbitmap *bmp,const u8 *data, s16 w, s16 h,
			   hwrcolor fg, hwrcolor bg);
struct fontglyph const *def_font_getglyph(struct fontdesc *fd, int ch);
g_error def_bitmap_getsize(hwrbitmap bmp,s16 *w,s16 *h);
void def_coord_keyrotate(s16 *k);
void rotate90_coord_keyrotate(s16 *k);
void rotate180_coord_keyrotate(s16 *k);
void rotate270_coord_keyrotate(s16 *k);
void def_coord_physicalize(s16 *x, s16 *y);
void rotate90_coord_physicalize(s16 *x, s16 *y);
void rotate180_coord_physicalize(s16 *x, s16 *y);
void rotate270_coord_physicalize(s16 *x, s16 *y);
void def_coord_logicalize(s16 *x, s16 *y);
void rotate90_coord_logicalize(s16 *x, s16 *y);
void rotate180_coord_logicalize(s16 *x, s16 *y);
void rotate270_coord_logicalize(s16 *x, s16 *y);
void linear32_pixel(hwrbitmap dest, s16 x, s16 y, hwrcolor c, s16 lgop);
hwrcolor linear32_getpixel(hwrbitmap src, s16 x, s16 y);
void linear32_slab(hwrbitmap dest, s16 x, s16 y, s16 w, hwrcolor c, s16 lgop);
void linear32_bar(hwrbitmap dest, s16 x,s16 y,s16 h,hwrcolor c, s16 lgop);
void linear32_rect(hwrbitmap dest, s16 x,s16 y,s16 w,s16 h,hwrcolor c, s16 lgop);
void linear32_blit(hwrbitmap dest, s16 x,s16 y,s16 w,s16 h, hwrbitmap src,
	      s16 src_x, s16 src_y, s16 lgop);

/************* Registration functions for video drivers */

g_error sdlfb_regfunc(struct vidlib *v);
g_error sdlgl_regfunc(struct vidlib *v);
g_error svgagl_regfunc(struct vidlib *v);
g_error svgafb_regfunc(struct vidlib *v);
g_error chipslice_video_regfunc(struct vidlib *v);
g_error ez328_regfunc(struct vidlib *v);
g_error ez328vga_regfunc(struct vidlib *v);
g_error ncurses_regfunc(struct vidlib *v);
g_error null_regfunc(struct vidlib *v);
g_error nullfb_regfunc(struct vidlib *v);
g_error fbdev_regfunc(struct vidlib *v);
g_error serial40x4_regfunc(struct vidlib *v);
g_error scrshot_regfunc(struct vidlib *v);
g_error s1d13806_regfunc(struct vidlib *v);
g_error sed133x_regfunc(struct vidlib *v);
g_error x11_regfunc(struct vidlib *v);
g_error mgl2fb_regfunc(struct vidlib *v);

/************** Registration functions for Video Base Libraries */
void setvbl_default(struct vidlib *vid);
void setvbl_linear1(struct vidlib *vid);
void setvbl_linear2(struct vidlib *vid);
void setvbl_linear4(struct vidlib *vid);
void setvbl_linear8(struct vidlib *vid);
void setvbl_linear16(struct vidlib *vid);
void setvbl_linear24(struct vidlib *vid);
void setvbl_linear32(struct vidlib *vid);
void setvbl_slowvbl(struct vidlib *vid);

/************** Registration functions for video wrapper libraries */
void vidwrap_rotate90(struct vidlib *vid);
void vidwrap_rotate180(struct vidlib *vid);
void vidwrap_rotate270(struct vidlib *vid);

/************** Bitmap format functions */
extern struct bitformat bitmap_formats[];

bool pnm_detect(const u8 *data, u32 datalen);
g_error pnm_load(hwrbitmap *bmp, const u8 *data, u32 datalen);

bool jpeg_detect(const u8 *data, u32 datalen);
g_error jpeg_load(hwrbitmap *bmp, const u8 *data, u32 datalen);

bool bmp_detect(const u8 *data, u32 datalen);
g_error bmp_load(hwrbitmap *bmp, const u8 *data, u32 datalen);

bool png_detect(const u8 *data, u32 datalen);
g_error png_load(hwrbitmap *bmp, const u8 *data, u32 datalen);

bool gif_detect(const u8 *data, u32 datalen);
g_error gif_load(hwrbitmap *bmp, const u8 *data, u32 datalen);

/* Runs the supplied function for all loaded bitmaps
 * (a superset of handle_iterate's results)
 */
g_error bitmap_iterate(g_error (*iterator)(hwrbitmap *pbit));

/************** Debugging */
void videotest_run(s16 number);
void videotest_help(void);
void videotest_benchmark(void);

/************** Send a driver message (to all loaded drivers) */

void drivermessage(u32 message, u32 param, u32 *ret);

#endif /* __H_VIDEO */

/* The End */
