/* $Id$
 *
 * video.h - Defines an API for writing PicoGUI video
 *           drivers
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

#ifndef __H_VIDEO
#define __H_VIDEO

#include <picogui/network.h>   /* For pgshmbitmap */
#include <pgserver/pgstring.h>

struct fontdesc;
struct pgquad;
struct pgrect;
struct pgpair;
struct groprender;
struct gropnode;
struct divtree;

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
 * (should be sufficient for most drivers)
 */
struct stdbitmap {
  u8  *bits;                 /* actual format depends on bpp */
  struct groprender *rend;   /* State for offscreen rendering */
  s16 w,h;
  u16 pitch;                 /* Spacing between lines, in bytes */
  u16 bpp;                   /* Bits per pixel of bitmap */

  int shm_id;                /* If nonzero, 'bits' is a shared memory segment.
			      * use this id to unmap it and remove when this
			      * bitmap is deleted.
			      */
  unsigned int freebits:1;   /* free() bits when deleting this bitmap */
};

#ifdef CONFIG_DITHER
/* This can be a hardware-specific dithering structure,
 * but usually it's a stddither pointer.
 */
typedef struct stddither *hwrdither;

/* Structure for keeping track of the state of an image being dithered.
 * This one is for the default floyd-steinberg dithering, but it can be
 * overridden. All buffers are indexed by channel.
 */
struct stddither {
  int *current_line[3];  /* Buffer for this line, pixel values * 16     */
  int *next_line[3];     /* Buffer for the next line                    */
  int *current[3];       /* Current position in current_line buffer     */
  int *below[3];         /* Current position in next_line buffer        */
  int x,y,w,h;           /* Destination rectangle                       */
  int dy;                /* Delta for y motion                          */
  int i;                 /* Iteration counter for the line              */
  hwrbitmap dest;        /* Where to render to                          */
};
#endif /* CONFIG_DITHER */

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
  handle dt; /* The divtree this sprite exists above */
  s16 x,y;   /* Current coordinates, relative to the display */
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

  /* IMPORTANT:
   *   when adding new primitives to the vidlib, update the rotation wrappers if necessary!
   */

  /******************************************** Initializing and video modes */

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
  void (*coord_logicalize)(int *x,int *y);

  /* Optional
   *   Converts logical coordinates back to physical
   *
   * Default implementation: does nothing
   */
  void (*coord_physicalize)(int *x,int *y);

  /* Optional
   *   For video wrappers, rotate keypads along with the display
   *
   * Default implementation: does nothing
   */
  void (*coord_keyrotate)(int *k);

  /* Reccomended (without double-buffer, it looks really dumb)
   *   Update changes to the screen, if device is double-buffered or
   *   uses page flipping
   *
   * Default implementation: does nothing (assumes changes are immediate)
   */
  void (*update)(hwrbitmap display, s16 x,s16 y,s16 w,s16 h);
   
  /* Current mode (only used in driver)
   *
   * The default bitmap functions should handle 1,2,4,8,16,24,32 bpp ok
   */
  s16 xres,yres,bpp;
  u32 flags;

  /* Logical screen size, read-only outside of driver.
   * Even in a rootless driver, this should return the size of the whole screen.
   */
  s16 lxres,lyres;
   
  /* fb_mem and fb_bpl are no longer here. Use display->bits and
   * display->pitch, also accessable with the macros FB_MEM and FB_BPL */
 
  /* This is provided for the video drivers' convenience only!
   * By default it is the bitmap indicating output to the display,
   * but the video driver may redefine it using the functions in the
   * window management section below.
   */
  hwrbitmap display;
   
  /* Optionally process driver messages */
  void (*message)(u32 message, u32 param, u32 *ret);


  /******************************************** Window management */

  /* The functions in this section are used by rootless mode.
   * A driver may support both rootless and monolithic modes:
   * rootless mode will be entered by calling setmode with the
   * PG_VID_ROOTLESS flag turned on. If the driver has been successfully
   * put into rootless mode, the is_rootless function should return 1.
   * In a driver that supports both rootless and conventional modes,
   * the user or package maintainer will select this via the app manager.
   */

  /* Optional
   *   Return a bitmap used for drawing debug information.
   *   On a normal driver, this should be one full screen.
   *   On a rootless driver, this should probably be a window created
   *   if it does not exist, and closed when the user requests.
   *
   *   The returned hwrbitmap will not be deleted. It may be deleted
   *   by the driver when the user requests so, as long as it is recreated
   *   the next time this function is called.
   *
   * Default implementation: returns vid->display
   */
  hwrbitmap (*window_debug)(void);  

  /* Optional
   *   Return a bitmap used for drawing to the whole screen.
   *   On a multi-display system, the display to use is at the
   *   discretion of the driver. (Probably the display with the highest
   *   resolution and color depth) On a rootless system this should be
   *   a new window created to cover all others, or possibly a full
   *   screen mode in the host system.
   *
   *   NOTE: This function is very likely to change when pgserver
   *         needs to get real multiple display support!
   *         This is also not really useful for fullscreen in rootless
   *         drivers yet, as there would need to be a way for the
   *         driver to know when to go in and out of fullscreen mode.
   *
   *   The returned hwrbitmap will not be deleted. It is expected
   *   to be a persistent resource.
   *
   * Default implementation: returns vid->display
   */
  hwrbitmap (*window_fullscreen)(void);  

  /* Optional
   *   In a rootless driver, create a new window and return a hwrbitmap
   *   describing it. The divtree this window is housing is specified,
   *   as it should be resized when the window is. The driver will need
   *   to save this divtree and use it when sending input events.
   *
   *   The returned hwrbitmap will be freed with window_free
   *
   * Default implementation: returns vid->display
   */
  g_error (*window_new)(hwrbitmap *bmp, struct divtree *dt);

  /* Optional
   *   In a rootless driver, create a new window and return a hwrbitmap
   *   describing it.
   *
   * Default implementation: does nothing
   */
  void (*window_free)(hwrbitmap window);

  /* Optional
   *   In a rootless driver, set the window title to the given string
   *
   * Default implementation: does nothing
   */
  void (*window_set_title)(hwrbitmap window, const struct pgstring *title);

  /* Optional
   *   In a rootless driver, set the window flags (PG_WINDOW_* constants)
   */
  void (*window_set_flags)(hwrbitmap window, int flags);

  /* Optional
   *   Routines to get/set position and size for the window
   *
   * Default implementation: set does nothing, get returns logical video mode
   */
  void (*window_set_position)(hwrbitmap window, s16 x, s16 y);
  void (*window_set_size)(hwrbitmap window, s16 w, s16 h);
  void (*window_get_position)(hwrbitmap window, s16 *x, s16 *y);
  void (*window_get_size)(hwrbitmap window, s16 *w, s16 *h);

  /* Optional
   *   Return nonzero if the driver is rootless. This indicates to the
   *   rendering engine that it needs to update all divtrees, not just the topmost one.
   *
   * Default implementation: return 0
   */
  int (*is_rootless)(void);


  /******************************************** Hooks */

  /* These hooks let picogui video drivers take over functionality
   * normally handled by other parts of picogui.
   */

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
			       struct groprender *rend, struct gropnode *node);

  /* Optional
   *   Called for any picogui update(). Return nonzero to abort the update
   */
  int (*update_hook)(void);

  /* Optional
   *   This lets the video driver add its own gropnodes. It's called whenever
   *   gropnode_draw finds an unknown grop type.
   */
  void (*grop_handler)(struct groprender *r, struct gropnode *n);

  /******************************************** Colors */

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

  /******************************************** Primitives */

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
   *   If the source and destination rectangles overlap, the result is undefined.
   *   This does _not_ tile or stretch bitmaps,
   *   so don't go past the edge of the source bitmap.
   * 
   * Default implementation: pixel!
   */
  void (*blit)(hwrbitmap dest, s16 x,s16 y,s16 w,s16 h, hwrbitmap src,
	       s16 src_x, s16 src_y, s16 lgop);

  /* Reccomended
   *   A version of blit() with the restriction on overlapping source
   *   and destination rectangles removed
   *   Also note that this does _not_ need to support tiling, and
   *   it is almost always called with PG_LGOP_NONE
   *
   * Default implementation: Multiple calls to blit()
   *                        (can be very slow in some directions)
   */
  void (*scrollblit)(hwrbitmap dest, s16 x,s16 y,s16 w,s16 h, hwrbitmap src,
		     s16 src_x, s16 src_y, s16 lgop);

  /* Reccomended
   *   Blits a bitmap or a section of a bitmap, wrapping around the edges of the
   *   source bitmap if necessary to cover the destination area.
   *   If the source and destination rectangles overlap, the result is undefined.
   *   x,y,w,h is the destination rectangle, sx,sy,sw,sh is the source rectangle,
   *   xo,yo is the amount of the first tile to skip in each axis. This function
   *   replaces the old tiling functionality of blit() and tileblit()
   *
   * Default implementation: Many calls to blit()!
   */
  void (*multiblit)(hwrbitmap dest, s16 x, s16 y, s16 w, s16 h,
		    hwrbitmap src, s16 sx, s16 sy, s16 sw, s16 sh, s16 xo, s16 yo, s16 lgop);

  /* Reccomended on platforms that are usually rotated
   *   Copy a source rectangle (in the source bitmap's coordinates)
   *   rotated to a new angle. The source x,y is always anchored at
   *   the destination x,y and rotation is performed anticlockwise about
   *   this point. This function is also responsible for clipping the rotated
   *   bitmap into that rectangle. On most platforms this will only support
   *   rotating by multiples of 90 degrees, but it may support arbitrary
   *   rotation as well. Note that this function is _not_ responsible
   *   for clipping the source rectangle, but since it is in normal
   *   bitmap coordinates it shouldn't be hard for the caller to do that
   *   if necessary.
   *
   * Default implementation: pixel()
   */
  void (*rotateblit)(hwrbitmap dest, s16 dest_x, s16 dest_y,
		     hwrbitmap src, s16 src_x, s16 src_y, s16 src_w, s16 src_h,
		     struct pgquad *clip, s16 angle, s16 lgop);
 
  void (*arc) (hwrbitmap dest, s16 x, s16 y, s16 w, s16 h,
	       s16 angle_start, s16 angle_stop, s16 angle_rot, hwrcolor color, s16 lgop);
  void (*ellipse) (hwrbitmap dest, s16 x, s16 y, s16 w, s16 h, hwrcolor c, s16 lgop); 
  void (*fellipse) (hwrbitmap dest, s16 x, s16 y, s16 w, s16 h, hwrcolor c, s16 lgop); 
  void (*fpolygon) (hwrbitmap dest, s32* array, s16 xoff, s16 yoff , hwrcolor c, s16 lgop);

  /* Optional
   *   Yep, it's a blur...
   */
  void (*blur) (hwrbitmap dest, s16 x, s16 y, s16 w, s16 h, s16 radius);
   
  /******************************************** Bitmaps */

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

  /* Optional
   *   Map the bitmap into shared memory, and fill out a struct pgshmbitmap
   *   structure with information about the bitmap. The 'shm' structure enters
   *   already zeroed, and should be returned with all applicable fields filled
   *   out in network byte order. 'uid' is the UID of the client that owns this
   *   bitmap.
   *
   * Default implementation: stdbitmap
   */
  g_error (*bitmap_getshm)(hwrbitmap bmp, u32 uid, struct pgshmbitmap *shm);

#ifdef CONFIG_DITHER

  /* Optional
   *   Start writing dithered data into a bitmap. Returns a dithering structure
   *   that's used to keep track of the position in the image. It's assumed that
   *   image data will be fed in top-down unless vflip is 1, in which case it's
   *   assumed bottom-up.
   */
  g_error (*dither_start)(hwrdither *d, hwrbitmap dest, int vflip, 
			  int x, int y, int w, int h);

  /* Optional
   *   Dither this pixel and store it in the bitmap
   */
  void (*dither_store)(hwrdither d, pgcolor pixel, s16 lgop);

  /* Optional
   *   Free the dithering structure
   */
  void (*dither_finish)(hwrdither d);

#endif /* CONFIG_DITHER */

  /******************************************** Sprites */

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
  void (*sprite_protectarea)(struct pgquad *in,struct sprite *from);


  /******************************************** Text/fonts */

  /* Reccomended when using BDF fonts
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
		   s16 lines, s16 angle, hwrcolor c, struct pgquad *clip,
		   s16 lgop, int char_pitch);

#ifdef CONFIG_FONTENGINE_FREETYPE
  /* Reccomended when using antialiased fonts
   *   Alpha blend the given color to the destination using 
   *   the supplied bytes as alpha mask.
   *   Origin, rotation, and clipping are handled just like charblit above.
   *   If gammatable is not NULL, use it for gamma correction.
   *
   * Default implementation: pixel
   */
  void (*alpha_charblit)(hwrbitmap dest, u8 *chardat, s16 x, s16 y, s16 w, s16 h,
			 int char_pitch, u8 *gammatable, s16 angle, hwrcolor c,
			 struct pgquad *clip, s16 lgop);
#endif
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
g_error new_sprite(struct sprite **ps,struct divtree *dt,s16 w,s16 h);
void free_sprite(struct sprite *s);

/* All sprites onscreen, sorted from back to front. */
extern struct sprite *spritelist;

/* Update region union/realize for a divtree */
void add_updarea(struct divtree *dt, s16 x,s16 y,s16 w,s16 h);
void realize_updareas(struct divtree *dt);

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
void def_update(hwrbitmap dest,s16 x, s16 y, s16 w, s16 h);
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
void def_multiblit(hwrbitmap dest, s16 x, s16 y, s16 w, s16 h,
		   hwrbitmap src, s16 sx, s16 sy, s16 sw, s16 sh, s16 xo, s16 yo, s16 lgop);
void def_charblit(hwrbitmap dest, u8 *chardat, s16 x, s16 y, s16 w, s16 h,
		  s16 lines, s16 angle, hwrcolor c, struct pgquad *clip,
		  s16 lgop, int char_pitch);
void def_alpha_charblit(hwrbitmap dest, u8 *chardat, s16 x, s16 y, s16 w, s16 h,
			int char_pitch, u8 *gammatable, s16 angle, hwrcolor c,
			struct pgquad *clip, s16 lgop);
void def_scrollblit(hwrbitmap dest, s16 x,s16 y,s16 w,s16 h, hwrbitmap src,
		    s16 src_x, s16 src_y, s16 lgop);
void def_rotateblit(hwrbitmap dest, s16 dest_x, s16 dest_y,
		    hwrbitmap src, s16 src_x, s16 src_y, s16 src_w, s16 src_h,
		    struct pgquad *clip, s16 angle, s16 lgop);
void def_sprite_protectarea(struct pgquad *in,struct sprite *from);
g_error def_bitmap_loadxbm(hwrbitmap *bmp,const u8 *data, s16 w, s16 h,
			   hwrcolor fg, hwrcolor bg);
struct fontglyph const *def_font_getglyph(struct fontdesc *fd, int ch);
g_error def_bitmap_getsize(hwrbitmap bmp,s16 *w,s16 *h);
g_error def_bitmap_getshm(hwrbitmap bmp, u32 uid, struct pgshmbitmap *shm);
void def_coord_keyrotate(int *k);
void rotate90_coord_keyrotate(int *k);
void rotate180_coord_keyrotate(int *k);
void rotate270_coord_keyrotate(int *k);
void def_coord_physicalize(int *x, int *y);
void rotate90_coord_physicalize(int *x, int *y);
void rotate180_coord_physicalize(int *x, int *y);
void rotate270_coord_physicalize(int *x, int *y);
void def_coord_logicalize(int *x, int *y);
void rotate90_coord_logicalize(int *x, int *y);
void rotate180_coord_logicalize(int *x, int *y);
void rotate270_coord_logicalize(int *x, int *y);
void linear32_pixel(hwrbitmap dest, s16 x, s16 y, hwrcolor c, s16 lgop);
hwrcolor linear32_getpixel(hwrbitmap src, s16 x, s16 y);
void linear32_slab(hwrbitmap dest, s16 x, s16 y, s16 w, hwrcolor c, s16 lgop);
void linear32_bar(hwrbitmap dest, s16 x,s16 y,s16 h,hwrcolor c, s16 lgop);
void linear32_rect(hwrbitmap dest, s16 x,s16 y,s16 w,s16 h,hwrcolor c, s16 lgop);
void linear32_blit(hwrbitmap dest, s16 x,s16 y,s16 w,s16 h, hwrbitmap src,
		   s16 src_x, s16 src_y, s16 lgop);
void yuv16_422_planar_pixel(hwrbitmap dest, s16 x, s16 y, hwrcolor c, s16 lgop);
hwrcolor yuv16_422_planar_getpixel(hwrbitmap src, s16 x, s16 y);
void yuv16_422_planar_blit(hwrbitmap dest, s16 x,s16 y,s16 w,s16 h, hwrbitmap src,
		   s16 src_x, s16 src_y, s16 lgop);
void def_arc(hwrbitmap dest, s16 x, s16 y, s16 w, s16 h,
	     s16 angle_start, s16 angle_stop, s16 angle_rot, hwrcolor color, s16 lgop);
void def_ellipse(hwrbitmap dest, s16 x, s16 y, s16 w, s16 h, hwrcolor c, s16 lgop); 
void def_fellipse(hwrbitmap dest, s16 x, s16 y, s16 w, s16 h, hwrcolor c, s16 lgop); 
void def_fpolygon(hwrbitmap dest, s32* array, s16 xoff, s16 yoff , hwrcolor c, s16 lgop);
g_error def_bitmap_load(hwrbitmap *bmp,const u8 *data,u32 datalen);
g_error def_bitmap_new(hwrbitmap *bmp,
		       s16 w,s16 h,u16 bpp);
void def_bitmap_free(hwrbitmap bmp);
void def_sprite_show(struct sprite *spr);
void def_sprite_hide(struct sprite *spr);
void def_sprite_update(struct sprite *spr);
void def_sprite_hideall(void);
void def_sprite_showall(void);
g_error def_bitmap_modeconvert(hwrbitmap *bmp);
g_error def_bitmap_modeunconvert(hwrbitmap *bmp);
g_error def_bitmap_get_groprender(hwrbitmap bmp, struct groprender **rend);
void def_blur(hwrbitmap dest, s16 x, s16 y, s16 w, s16 h, s16 radius);
#ifdef CONFIG_DITHER
g_error def_dither_start(hwrdither *d, hwrbitmap dest, int vflip, 
			 int x, int y, int w, int h);
void def_dither_store(hwrdither d, pgcolor pixel, s16 lgop);
void def_dither_finish(hwrdither d);
#endif


/**************************************** Window flags */

#define PG_WINDOW_UNMANAGED   (1<<0)      /* This window should not be decorated or managed by the host GUI */
#define PG_WINDOW_GRAB        (1<<1)      /* This window grabs all user input while it exists */
#define PG_WINDOW_BACKGROUND  (1<<2)      /* This window stays behind all others and takes the entire screen */


/**************************************** Registration functions for video drivers */

g_error sdlfb_regfunc(struct vidlib *v);
g_error svgagl_regfunc(struct vidlib *v);
g_error sdlgl_regfunc(struct vidlib *v);
g_error glcontext_regfunc(struct vidlib *v);
g_error svgafb_regfunc(struct vidlib *v);
g_error chipslice_video_regfunc(struct vidlib *v);
g_error ez328_regfunc(struct vidlib *v);
g_error ez328vga_regfunc(struct vidlib *v);
g_error ncurses_regfunc(struct vidlib *v);
g_error null_regfunc(struct vidlib *v);
g_error nullfb_regfunc(struct vidlib *v);
g_error fbdev_regfunc(struct vidlib *v);
g_error yuv16_422_planar_regfunc(struct vidlib *v);
g_error serial40x4_regfunc(struct vidlib *v);
g_error scrshot_regfunc(struct vidlib *v);
g_error s1d13806_regfunc(struct vidlib *v);
g_error sed133x_regfunc(struct vidlib *v);
g_error x11_regfunc(struct vidlib *v);
g_error mgl2fb_regfunc(struct vidlib *v);
g_error vncserver_regfunc(struct vidlib *v);
g_error directfb_regfunc(struct vidlib *v);

/***************************************** Registration functions for Video Base Libraries */
void setvbl_default(struct vidlib *vid);
void setvbl_linear1(struct vidlib *vid);
void setvbl_linear2(struct vidlib *vid);
void setvbl_linear4(struct vidlib *vid);
void setvbl_linear8(struct vidlib *vid);
void setvbl_linear16(struct vidlib *vid);
void setvbl_linear24(struct vidlib *vid);
void setvbl_linear32(struct vidlib *vid);
void setvbl_yuv16_422_planar(struct vidlib *vid);
void setvbl_slowvbl(struct vidlib *vid);
void setvbl_gl(struct vidlib *vid);

/***************************************** Registration functions for video wrapper libraries */
void vidwrap_rotate90(struct vidlib *vid);
void vidwrap_rotate180(struct vidlib *vid);
void vidwrap_rotate270(struct vidlib *vid);

/***************************************** Bitmap format functions */
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

/* Rotate an existing bitmap by the given angle, reallocating it.
 * Only supports angles 0,90,180,270
 */
g_error bitmap_rotate(hwrbitmap *pbit, s16 angle);

/* Run the given function on _all_ bitmaps */
g_error bitmap_iterate(handle_iterator iterator, void *extra);

/* Rotate _all_ loaded bitmaps by the given angle */
g_error bitmap_rotate_all(s16 angle);


/***************************************** Initialization */

g_error video_init(void);
void video_shutdown(void);


/***************************************** Debugging */

void videotest_run(s16 number);
void videotest_help(void);
void videotest_benchmark(void);


/***************************************** Send a driver message (to all loaded drivers) */

void drivermessage(u32 message, u32 param, u32 *ret);


/***************************************** Palettes */

#ifdef CONFIG_PAL8_CUSTOM
extern pgcolor palette8_custom[256];
g_error load_custom_palette(const char *name);
#endif

/* Default heuristics for reporting color information in SHM bitmaps */
void def_shm_colorspace(int bpp, int alpha, struct pgshmbitmap *shm);

#endif /* __H_VIDEO */

/* The End */
