/* $Id: video.h,v 1.12 2000/12/16 20:08:45 micahjd Exp $
 *
 * video.h - Defines an API for writing PicoGUI video
 *           drivers
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000 Micah Dowty <micahjd@users.sourceforge.net>
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

/* Hardware-specific bitmap */
typedef void * hwrbitmap;

/* The hwrbitmap used in the default implementation
   (should be sufficient for most drivers)
*/
struct stdbitmap {
  unsigned char *bits;    /* actual format depends on bpp */
  int w,h;
  int freebits;    /* Should 'bits' be freed also when bitmap is freed? */
};

/* A sprite node, overlaid on the actual picture */
struct sprite {
  hwrbitmap *bitmap, *mask, *backbuffer;
  int x,y;   /* Current coordinates */
  int ox,oy; /* Coordinates last time it was drawn */
  int w,h;   /* Dimensions of all buffers */
  struct divnode *clip_to;
  struct sprite *next;
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

  /* Current mode (read only to all but driver)
   *
   * The default bitmap functions should handle 1,2,4,8,16,24,32 bpp ok
   */
  int xres,yres,bpp;
  unsigned long flags;

  /* Framebuffer information (for framebuffer Video Base Libraries) */
  unsigned char *fb_mem;
  unsigned int fb_bpl;   /* Bytes Per Line */

  /***************** Clipping */

  /* Reccomended
   *   Set a new clipping rectangle
   *
   * Default implementation: just stores it
   */
  void (*clip_set)(int x1,int y1,int x2,int y2);

  /* Reccomended
   *   Turns off clipping
   *
   * Default implementation: sets clip to full screen
   */
  void (*clip_off)(void);

  /* Current clipping (read only to all but driver) */
  int clip_x1,clip_y1,clip_x2,clip_y2;

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
  void (*update)(void);

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
   *   Frames an area with a hollow rectangle
   *
   * Default implementation: uses slab() and bar()
   */
  void (*frame)(int x,int y,int w,int h,hwrcolor c);

  /* Optional
   *   Dims or 'grays out' everything in the clipping rectangle
   *
   * Default implementation: color #0 checkerboard pattern
   */
  void (*dim)(void);

  /* Required (The alternative, pixel(), is just too scary)
   *   Blits a bitmap to screen, optionally using lgop.
   *   If w and/or h is bigger than the source bitmap, it
   *   should tile.
   */
  void (*blit)(hwrbitmap src,int src_x,int src_y,
	       int dest_x,int dest_y,
	       int w,int h,int lgop);

  /* Required (Same reason as blit)
   *   Blits a chunk of the screen back to a bitmap
   */
  void (*unblit)(int src_x,int src_y,
		 hwrbitmap dest,int dest_x,int dest_y,
		 int w,int h);

  /* Optional
   *   Does a bottom-up blit from an area on the screen
   *   to an area on the screen. Used for scrolling down.
   *   This doesn't need to worry about clipping 
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
		   hwrcolor c);
     
  /* Optional
   *   Like charblit, but rotates the character 90 degrees counterclockwise
   *   for displaying vertical text.
   *
   * Default implementation: pixel()...
   */
  void (*charblit_v)(unsigned char *chardat,int dest_x,
		     int dest_y,int w,int h,int lines,
		     hwrcolor c);

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

  /* XBM 1-bit data */
  g_error (*bitmap_loadxbm)(hwrbitmap *bmp,
			    unsigned char *data,
			    int w,int h,
			    hwrcolor fg,
			    hwrcolor bg);

  /* PNM 8/16/24-bit portable graphics */
  g_error (*bitmap_loadpnm)(hwrbitmap *bmp,
			    unsigned char *data,
			    unsigned long datalen);

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

};

/* Currently in-use video driver */
extern struct vidlib *vid;

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

/* Registration functions */
g_error sdlfb_regfunc(struct vidlib *v);
g_error sdl_regfunc(struct vidlib *v);
g_error svga_regfunc(struct vidlib *v);
g_error chipslice_video_regfunc(struct vidlib *v);

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
extern unsigned char sprites_hidden;

/* Helper functions for keeping an update region, used
   for double-buffering by the video drivers */
extern int upd_x;
extern int upd_y;
extern int upd_w;
extern int upd_h;
void add_updarea(int x,int y,int w,int h);

hwrcolor textcolors[16];   /* Table for converting 16 text colors
			      to hardware colors */

/** Generic functions from the default VBL that other VBLs might find useful */

g_error def_setmode(int xres,int yres,int bpp,unsigned long flags);
void emulate_dos(void);
void def_clip_set(int x1,int y1,int x2,int y2);
void def_clip_off(void);
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
void def_frame(int x,int y,int w,int h,hwrcolor c);
void def_dim(void);
void def_scrollblit(int src_x,int src_y,int dest_x,int dest_y,int w,int h);
void def_charblit(unsigned char *chardat,int dest_x,
		  int dest_y,int w,int h,int lines,hwrcolor c);
void def_charblit_v(unsigned char *chardat,int dest_x,
		    int dest_y,int w,int h,int lines,hwrcolor c);
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

/************** Registration functions for Video Base Libraries */
void setvbl_default(struct vidlib *vid);
void setvbl_linear8(struct vidlib *vid);

#endif /* __H_VIDEO */

/* The End */







