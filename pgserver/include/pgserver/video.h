/* $Id: video.h,v 1.30 2001/03/20 00:46:43 micahjd Exp $
 *
 * video.h - Defines an API for writing PicoGUI video
 *           drivers
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000,2001 Micah Dowty <micahjd@users.sourceforge.net>
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

/* Hardware-specific color value */
typedef unsigned long hwrcolor;

/* PicoGUI color (24-bit RGB)
   Usually converted to a hwrcolor at the first opportunity
*/
typedef unsigned long pgcolor;
#define getred(pgc)    (((pgc)>>16)&0xFF)
#define getgreen(pgc)  (((pgc)>>8)&0xFF)
#define getblue(pgc)   ((pgc)&0xFF)
#define mkcolor(r,g,b) (((r)<<16)|((g)<<8)|(b))

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
  u8  *bits;    /* actual format depends on bpp */
  s16 w,h;
  u16 pitch;       /* Spacing between lines, in bytes */

  /* Should 'bits' be freed also when bitmap is freed? */
  unsigned int freebits : 1;    
};

/* A group of functions to deal with one bitmap format */
struct bitformat {
  char name[4];     /* fourcc for the format name */

  int (*detect)(u8 *data, u32 datalen);
  g_error (*load)(struct stdbitmap **bmp, u8 *data, u32 datalen);
  g_error (*save)(struct stdbitmap *bmp, u8 **data, u32 *datalen);
};
   
/* A sprite node, overlaid on the actual picture */
struct sprite {
  hwrbitmap bitmap,mask,backbuffer;
  int x,y;   /* Current coordinates */
  int ox,oy; /* Coordinates last time it was drawn */
  int w,h;   /* Dimensions of all buffers */
  int ow,oh; /* The blit last time, with clipping */
  struct divnode *clip_to;
  struct sprite *next;
  unsigned int onscreen : 1;   /* Displayed on screen now */
  unsigned int visible  : 1;   /* Displayed under normal circumstances */
};
/* List of sprites to overlay */
extern struct sprite *spritelist;

/* The text clipping is generally more complex than it would be
 * fun to do in grop.c, so a clipping rectangle is passed along for them.
 */
struct cliprect {
   signed short int x1,y1,x2,y2;
};

/* NOTE: font.h must be included here. It relies on structures defined
 * earlier, but font_* below need font.h.
 * 
 * Messy, isn't it...
 */
#include <pgserver/font.h>

/* This structure contains a pointer to each graphics function
   in use, forming a definition for a driver. Initially, all functions
   point to default implementations. It is only necessary for a driver
   to implement a few functions, but it can optionally implement others
   if they can be accelerated
*/

struct vidlib {

  /***************** Initializing and video modes */

  /* Required
   *   initializes graphics device, given a default mode.
   *   If the driver doesn't support that mode it can choose something
   *   else, or even ignore it all together. 
   */
  g_error (*init)(int xres,int yres,int bpp,unsigned long flags);

  /* Reccomended if the device supports mode switching
   *   Changes the video mode after initialization
   *
   * Default implementation: returns error
   */
  g_error (*setmode)(int xres,int yres,int bpp,unsigned long flags);

  /* Reccomended
   *   free memory, close device, etc. 
   * 
   * Default implementation: does nothing
   */
  void (*close)(void);

  /* Optional
   *   Converts physical coordinates (from an input device)
   *   into logical coordinates
   */
  void (*coord_logicalize)(int *x,int *y);
   
  /* Current mode (only used in driver)
   *
   * The default bitmap functions should handle 1,2,4,8,16,24,32 bpp ok
   */
  int xres,yres,bpp;
  unsigned long flags;

  /* Logical screen size, read-only outside of driver */
  int lxres,lyres;
   
  /* Framebuffer information (for framebuffer Video Base Libraries) */
  unsigned char *fb_mem;
  unsigned int fb_bpl;   /* Bytes Per Line */

  /***************** Fonts */
   
  /* Optional
   *   Called when a new fontdesc is created. The video driver
   *   may choose to modify the font or cache things or something
   * 
   * Default implementation: none
   */
  void (*font_newdesc)(struct fontdesc *fd);
  
   
  /***************** Colors */

  /* Reccomended
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
  void (*pixel)(int x,int y,hwrcolor c);

  /* Required
   *   Get a pixel, in hwrcolor format
   */
  hwrcolor (*getpixel)(int x,int y);

  /* Reccomended
   *   Add/subtract the color from the
   *   existing pixel
   *
   * Default implementation: getpixel, modifies it, then putpixel
   */
  void (*addpixel)(int x,int y,pgcolor c);
  void (*subpixel)(int x,int y,pgcolor c);

  /* Optional
   *   clear screen
   *   
   * Default implementation: draws a rectangle of 
   *   color 0 over the screen
   */
  void (*clear)(void);

  /* Reccomended (without double-buffer, it looks really dumb)
   *   Update changes to the screen, if device is double-buffered or
   *   uses page flipping
   *
   * Default implementation: does nothing (assumes changes are immediate)
   */
  void (*update)(int x,int y,int w,int h);

  /* Optional
   *   draws a continuous horizontal run of pixels
   *
   * Default implementation: draws a 1 pixel high rectangle
   */
  void (*slab)(int x,int y,int w,hwrcolor c);

  /* Optional
   *   draws a vertical bar of pixels (a sideways slab)
   *
   * Default implementation: draws a 1 pixel wide rectangle
   */
  void (*bar)(int x,int y,int h,hwrcolor c);

  /* Optional
   *   draws an arbitrary line between 2 points
   *
   * Default implementation: bresenham algrorithm and
   *   many calls to pixel()
   */
  void (*line)(int x1,int y1,int x2,int y2,hwrcolor c);

  /* Reccomended
   *   fills a rectangular area with a color
   *
   * Default implementation: for loops and many pixel()s
   */
  void (*rect)(int x,int y,int w,int h,hwrcolor c);

  /* Reccomended for high color/true color devices
   *   fills a rectangular area with a linear gradient
   *   of an arbitrary angle (in degrees), between two colors.
   *   If angle==0, c1 is at the left and c2 is at the
   *   right.  The gradient rotates clockwise as angle
   *   increases.
   *   If translucent==0, the gradient overwrites existing
   *   stuff on the screen. -1, it subtracts, and +1, it 
   *   adds.
   *
   * Default implementation: interpolation algorithm, pixel()
   */
  void (*gradient)(int x,int y,int w,int h,int angle,
		   pgcolor c1, pgcolor c2,int translucent);
  
  /* Optional
   *   Dims or 'grays out' everything in the rectangle
   *
   * Default implementation: color #0 checkerboard pattern
   */
  void (*dim)(int x,int y,int w,int h);

  /* Very Reccomended
   *   Blits a bitmap to screen, optionally using lgop.
   *   If w and/or h is bigger than the source bitmap, it
   *   should tile.
   * 
   * Default implementation: pixel!
   */
  void (*blit)(hwrbitmap src,int src_x,int src_y,
	       int dest_x,int dest_y,
	       int w,int h,int lgop);

  /* Very Reccomended
   *   Blits a chunk of the screen back to a bitmap
   *
   * Default implementation: pixel!
   */
  void (*unblit)(int src_x,int src_y,
		 hwrbitmap dest,int dest_x,int dest_y,
		 int w,int h);

  /* Optional
   *   Does a bottom-up blit from an area on the screen
   *   to an area on the screen. Used for scrolling down.
   *
   * Default implementation: Calls blit() for every line,
   *   starting at the bottom
   */
  void (*scrollblit)(int src_x,int src_y,
		     int dest_x,int dest_y,
		     int w,int h);

  /* Reccomended
   *   Blits a bitmap or a section of a bitmap repeatedly
   *   to cover an area. Used by many bitmap themes.
   *
   * Default implementation: Many calls to blit()!
   */
  void (*tileblit)(hwrbitmap src,
		   int src_x,int src_y,int src_w,int src_h,
		   int dest_x,int dest_y,int dest_w,int dest_h);

  /* Reccomended
   *   Used for character data.  Blits 1bpp data from
   *   chardat to the screen, filling '1' bits with the
   *   color 'col'.  If lines > 0, every 'lines' lines
   *   the image is left-shifted one pixel, to simulate
   *   italics
   *
   * Default implementation: pixel(). Need I say more?
   */
  void (*charblit)(unsigned char *chardat,int dest_x,
		   int dest_y,int w,int h,int lines,
		   hwrcolor c,struct cliprect *clip);
     
  /* Optional
   *   Like charblit, but rotates the character 90 degrees counterclockwise
   *   for displaying vertical text.
   *
   * Default implementation: pixel()...
   */
  void (*charblit_v)(unsigned char *chardat,int dest_x,
		     int dest_y,int w,int h,int lines,
		     hwrcolor c,struct cliprect *clip);

  /***************** Bitmaps */

  /* These functions all use the stdbitmap structure.
     If your driver needs a different bitmap format,
     implement all these functions.  If not, you should
     be ok leaving these with the defaults.
  */

  /* Optional
   *   Allocates a new bitmap, and loads the
   *   formatted bitmap data into it
   *
   * Default implementation: uses stdbitmap structure
   */

#ifdef CONFIG_FORMAT_XBM
  /* XBM 1-bit data (used internally) */
  g_error (*bitmap_loadxbm)(hwrbitmap *bmp,u8 *data, s16 w, s16 h,
			    hwrcolor fg,hwrcolor bg);
#endif
   
  /* Load a bitmap, detecting the appropriate format */
  g_error (*bitmap_load)(hwrbitmap *bmp,u8 *data,u32 datalen);

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
			int w,int h);

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
  g_error (*bitmap_getsize)(hwrbitmap bmp,int *w,int *h);

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
  void (*sprite_protectarea)(struct cliprect *in,struct sprite *from);
   
};

/* Currently in-use video driver */
extern struct vidlib *vid;

/* Optional wrapper around that driver that provides some transformation */
extern struct vidlib *vidwrap;

/* This macro is used to call a video function */
#define VID(f) (*vidwrap->f)

/* Trig (sin*256 from 0 to 90 degrees) */
extern unsigned char trigtab[];

/* Some helper functions for PNM files */
void ascskip(unsigned char **dat,unsigned long *datlen);
int ascread(unsigned char **dat,unsigned long *datlen);

/*
  Unloads previous driver, sets up a new driver and
  initializes it.  This is for changing the driver,
  and optionally changing the mode.  If you just
  want to change the mode use vid->setmode
*/
g_error load_vidlib(g_error (*regfunc)(struct vidlib *v),
		    int xres,int yres,int bpp,unsigned long flags);

/* List of installed video drivers */
struct vidinfo {
  char *name;
  g_error (*regfunc)(struct vidlib *v);
};
extern struct vidinfo videodrivers[];

g_error (*find_videodriver(const char *name))(struct vidlib *v);

/* Sprite helper functions */
g_error new_sprite(struct sprite **ps,int w,int h);
void free_sprite(struct sprite *s);

/* Sprite vars */
extern struct sprite *spritelist;

/* Helper functions for keeping an update region, used
   for double-buffering by the video drivers */
extern int upd_x;
extern int upd_y;
extern int upd_w;
extern int upd_h;
void add_updarea(int x,int y,int w,int h);
void realize_updareas(void);

hwrcolor textcolors[16];   /* Table for converting 16 text colors
			      to hardware colors */

/** Generic functions from the default VBL that other VBLs might find useful */

g_error def_setmode(int xres,int yres,int bpp,unsigned long flags);
void def_font_newdesc(struct fontdesc *fd);
void emulate_dos(void);
void def_update(int x,int y,int w,int h);
hwrcolor def_color_pgtohwr(pgcolor c);
pgcolor def_color_hwrtopg(hwrcolor c);
void def_addpixel(int x,int y,pgcolor c);
void def_subpixel(int x,int y,pgcolor c);
void def_clear(void);
void def_slab(int x,int y,int w,hwrcolor c);
void def_bar(int x,int y,int h,hwrcolor c);
void def_line(int x1,int y1,int x2,int y2,hwrcolor c);
void def_rect(int x,int y,int w,int h,hwrcolor c);
void def_gradient(int x,int y,int w,int h,int angle,
		  pgcolor c1, pgcolor c2,int translucent);
void def_dim(int x,int y,int w,int h);
void def_scrollblit(int src_x,int src_y,int dest_x,int dest_y,int w,int h);
void def_charblit(unsigned char *chardat,int dest_x,
		  int dest_y,int w,int h,int lines,hwrcolor c,
		  struct cliprect *clip);
void def_charblit_v(unsigned char *chardat,int dest_x,
		    int dest_y,int w,int h,int lines,hwrcolor c,
		    struct cliprect *clip);
g_error def_bitmap_loadxbm(struct stdbitmap **bmp,unsigned char *data,
			   int w,int h,hwrcolor fg,hwrcolor bg);
g_error def_bitmap_loadpnm(struct stdbitmap **bmp,unsigned char *data,
			   unsigned long datalen);
g_error def_bitmap_new(struct stdbitmap **bmp,int w,int h);
void def_bitmap_free(struct stdbitmap *bmp);
g_error def_bitmap_getsize(struct stdbitmap *bmp,int *w,int *h);
void def_tileblit(struct stdbitmap *src,
		  int src_x,int src_y,int src_w,int src_h,
		  int dest_x,int dest_y,int dest_w,int dest_h);
void def_sprite_show(struct sprite *spr);
void def_sprite_hide(struct sprite *spr);
void def_sprite_update(struct sprite *spr);
void def_sprite_showall(void);
void def_sprite_hideall(void);
void def_sprite_protectarea(struct cliprect *in,struct sprite *from);
void def_blit(hwrbitmap src,int src_x,int src_y,
	      int dest_x,int dest_y,
	      int w,int h,int lgop);
void def_unblit(int src_x,int src_y,
		hwrbitmap dest,int dest_x,int dest_y,
		int w,int h);
g_error def_bitmap_rotate90(hwrbitmap *bmp);

/************* Registration functions for video drivers */

g_error sdlfb_regfunc(struct vidlib *v);
g_error sdl_regfunc(struct vidlib *v);
g_error svgagl_regfunc(struct vidlib *v);
g_error svgafb_regfunc(struct vidlib *v);
g_error chipslice_video_regfunc(struct vidlib *v);
g_error ez328_regfunc(struct vidlib *v);
g_error ncurses_regfunc(struct vidlib *v);
g_error null_regfunc(struct vidlib *v);
g_error fbdev_regfunc(struct vidlib *v);

/************** Registration functions for Video Base Libraries */
void setvbl_default(struct vidlib *vid);
void setvbl_linear1(struct vidlib *vid);
void setvbl_linear2(struct vidlib *vid);
void setvbl_linear4(struct vidlib *vid);
void setvbl_linear8(struct vidlib *vid);
void setvbl_linear16(struct vidlib *vid);

/************** Registration functions for video wrapper libraries */
void vidwrap_rotate90(struct vidlib *vid);

/************** Bitmap format functions */
extern struct bitformat bitmap_formats[];

int pnm_detect(u8 *data, u32 datalen);
g_error pnm_load(struct stdbitmap **bmp, u8 *data, u32 datalen);

/************** Debugging */
void videotest_run(int number);
void videotest_help(void);

#endif /* __H_VIDEO */

/* The End */







