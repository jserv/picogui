/* $Id: fbdev.c,v 1.34 2002/03/29 13:21:18 micahjd Exp $
 *
 * fbdev.c - Some glue to use the linear VBLs on /dev/fb*
 * 
 * This driver supports 8,16,24, and 32 bit color at any resolution.
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
 *  A lot of the VT switching code here was based on Microwindows.
 *    Microwindows is Copyright (c) 2001 Century Software, Inc.
 *
 */

#include <pgserver/common.h>

#include <pgserver/video.h>
#include <pgserver/render.h>
#include <pgserver/configfile.h>
#include <pgserver/timer.h>
#include <pgserver/input.h>

#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <linux/vt.h>
#include <linux/kd.h>
#include <linux/vt.h>
#include <signal.h>

#ifdef CONFIG_FB_YUV16
# include "yuv.h"
#endif

#ifdef DEBUG_VT
#define DEBUG_FILE
#endif
#include <pgserver/debug.h>

/**************************************** Globals */

/* Macros to easily access the members of vid->display */
#define FB_MEM   (((struct stdbitmap*)vid->display)->bits)
#define FB_BPL   (((struct stdbitmap*)vid->display)->pitch)
#define FB_TYPE(type,bpp)  (((type)<<8)|(bpp))

#define DEFAULT_FB   "/dev/fb0"
#define DEFAULT_TTY  "/dev/tty0"

/* This information is only saved so we can munmap() and close()... */
int fbdev_fd;
u32 fbdev_mapsize;
int ttyfd;

/* Save screen info for color conversion */
struct fb_fix_screeninfo fixinfo;
struct fb_var_screeninfo varinfo;

#ifdef CONFIG_FIX_VR3
//Color map for the Agenda VR3
static u16 vr_lcd_intensity[16] = {
    0x0000,
    0x1111,
    0x2222,
    0x3333,
    0x4444,
    0x5555,
    0x6666,
    0x7777,
    0x8888,
    0x9999,
    0xaaaa,
    0xbbbb,
    0xcccc,
    0xdddd,
    0xeeee,
    0xffff
};
#endif

/* Saved palette */
static short fbdev_saved_r[16];
static short fbdev_saved_g[16];
static short fbdev_saved_b[16];

/* If this is non-null, we're doing double buffering and this is the
 * real screen while vid->display is the backbuffer.
 */
hwrbitmap screen_buffer;

#ifdef CONFIG_FB_PAGEFLIP
/* If this is 1, then screen_buffer and vid->display are flipped */
int fbdev_flipped;
#endif

void fbdev_doublebuffer_update(s16 x,s16 y,s16 w,s16 h);
void fbdev_close(void);

/**************************************** Color conversion */

/* Our own conversion routines for true color
 */
pgcolor fbdev_color_hwrtopg(hwrcolor c) {
  if (c & PGCF_MASK) return def_color_hwrtopg(c);

  return mkcolor( (u8)((c >> varinfo.red.offset  ) << (8 - varinfo.red.length  )),
		  (u8)((c >> varinfo.green.offset) << (8 - varinfo.green.length)),
		  (u8)((c >> varinfo.blue.offset ) << (8 - varinfo.blue.length )) );

}
hwrcolor fbdev_color_pgtohwr(pgcolor c) {
  if (c & PGCF_MASK) return def_color_pgtohwr(c);

  return ( (((u32)getred(c))   >> (8 - varinfo.red.length  )) << varinfo.red.offset   ) |
         ( (((u32)getgreen(c)) >> (8 - varinfo.green.length)) << varinfo.green.offset ) |
         ( (((u32)getblue(c))  >> (8 - varinfo.blue.length )) << varinfo.blue.offset  );
}

/**************************************** YUV memory format */

#ifdef CONFIG_FB_YUV16

/* The length of the Y plane */
static size_t yuv16_y_seg_size  = 0;
static size_t yuv16_y_seg_pitch = 0;

/* The RGB shadow buffer.
 *
 * Note: we are not using the double buffer, because it is working
 * on an update() base, and we want to buffer on a pixel() base.
 * Our shadow buffer is here for getpixel() to avoid having to do
 * a YUV->RGB conversion.
 */
hwrcolor* yuv_rgb_shadow_buffer = 0;


/* Our own conversion routines for YUV 16
 */
pgcolor yuv16_color_hwrtopg(hwrcolor c)
{
  /* hwrcolor is in RGB until very hardware access */
  return c;
}

hwrcolor yuv16_color_pgtohwr(pgcolor c)
{
  /* hwrcolor is in RGB until very hardware access */
  return c;
}

static inline int fbdev_yuv16_is_offscreen(u8* bits)
{
  return bits<FB_MEM || bits>=FB_MEM+yuv16_y_seg_size;
}

static hwrcolor fbdev_yuv16_getpixel(hwrbitmap src, s16 x, s16 y)
{
  struct stdbitmap *bmp = (struct stdbitmap *)src;
  u8* bits = bmp->bits;

  if( fbdev_yuv16_is_offscreen(bits) ) {
    /* we are offscreen */
    return def_getpixel(src, x, y);
  }
  else {
    size_t offset = yuv16_y_seg_pitch*y + x;
    return yuv_rgb_shadow_buffer[offset];
  }
}

static void fbdev_yuv16_pixel(hwrbitmap dest,
			      s16 x, s16 y, hwrcolor c,
			      s16 lgop)
{
  /* no special check: if we are here, it is because we are in YUV16 */

  struct stdbitmap *bmp = (struct stdbitmap *) dest;
  u8* bits = bmp->bits;

  if( fbdev_yuv16_is_offscreen(bits) ) {
    /* we are offscreen */
    def_pixel(dest, x, y, c, lgop);
    return;
  }
  else {
    unsigned long r = getred(c);
    unsigned long g = getgreen(c);
    unsigned long b = getblue(c);

    size_t offset = yuv16_y_seg_pitch*y + x;
    
    u8 *dst_y  = FB_MEM + offset;
    u8 *dst_uv = FB_MEM + yuv16_y_seg_size + (offset&~1);

    switch (lgop) {      
    case PG_LGOP_NONE: {
      int y, cb, cr;
      
      yuv_rgb_shadow_buffer[offset] = c;
      rgb_to_ycbcr(r, g, b, &y, &cb, &cr);
      *dst_y = (char)y;
      *dst_uv++ = (char)cb;
      *dst_uv = (char)cr;
      
      break; 
    }
    
    default:
    /* Not supported yet */
      return;
    }
  }
}

#endif


/**************************************** Virtual Terminals */
#ifdef CONFIG_FB_VT

/* This is set when the SIGVT handler should be active */
volatile u8 fbdev_handler_on = 0;

/* This uses the null driver to disable all rendering.
 * Here we store the former video driver so we can restore it.
 */
struct vidlib fbdev_savedlib;

/* The original virtual terminal, before pgserver started */
int fbdev_savedvt;

/* The VT that this pgserver is using */
int fbdev_pgvt;

/* Signal to use for VT switching */
#define SIGVT SIGUSR1

/* This is a hack to get the console to realign the virtual screen's origin
 * with the physical screen. Thanks Microwindows, i probably wouldn't have
 * figured this one out on my own :)
 *
 * FIXME: As microwindows' vtswitch.c states, this is a horrible hack but it works
 */
void fbdev_redrawvt(int vt) {
  DBG("%d\n",vt);
  ioctl(ttyfd, VT_ACTIVATE, vt == 1 ? 2 : 1);
  ioctl(ttyfd, VT_ACTIVATE, vt);
}

/* Get the current VT 
 */
int fbdev_getvt(void) {
  struct vt_stat stat;
  ioctl(ttyfd, VT_GETSTATE, &stat);
  return stat.v_active;
}

/* enable/disable graphical output */
void fbdev_enable(void) {
  struct divtree *p;
  disable_output = 0;
  disable_timers = 0;
  disable_input  = 0;
  
  DBG("on VT %d\n",fbdev_getvt());

  for (p=dts->top;p;p=p->next)
    p->flags |= DIVTREE_ALL_REDRAW;
  update(NULL,1);
}
void fbdev_disable(void) {
  DBG("on VT %d\n",fbdev_getvt());

  inactivity_reset();
  disable_output = 1;
  disable_timers = 1;
  disable_input = 1;
}

/* Indirectly, this is the signal handler. A few extra
 * signals like SIGUSR1 get routed here
 */
void fbdev_message(u32 message, u32 param, u32 *ret) {
  static int disabled = 0;
  
  DBG("Got message %d, param %d\n",message,param);

  if (message!=PGDM_SIGNAL || param!=SIGVT || !fbdev_handler_on)
    return;

  /* Toggle in and out of our VT.. 
   * Set variables, redraw, acknowledge
   */
  if (disabled) {
    ioctl(ttyfd, VT_RELDISP, VT_ACKACQ);
    fbdev_enable();
  }
  else {
    fbdev_disable();
    ioctl(ttyfd, VT_RELDISP, 1);
  }

  disabled = !disabled;
}

/* Switch to the right VT, set up stuff */
g_error fbdev_initvt(void) {
  const char *vt;
  char buf[20];

  fbdev_savedvt = fbdev_getvt();
  
  /* We'll need /dev/tty0 just to determine what VT to run on
   */
  ttyfd = open("/dev/tty0", O_RDWR);
  if (ttyfd <= 0)
    return mkerror(PG_ERRT_IO,107);   /* can't open TTY */
  
  /* What VT should we run on?
   */
  vt = get_param_str("video-fbdev","vt","current");
  if (!strcmp(vt,"current"))
    fbdev_pgvt = fbdev_savedvt;
  else if (!strcmp(vt,"auto"))
    ioctl(ttyfd, VT_OPENQRY, &fbdev_pgvt);
  else
    fbdev_pgvt = atoi(vt);

  DBG("video-fbdev.vt = %s -> %d\n",vt,fbdev_pgvt);
    
  ioctl(ttyfd, VT_ACTIVATE, fbdev_pgvt);

  /* Redraw the text mode to put us back at the
   * top of the virtual screen
   */
  if (fbdev_pgvt == fbdev_savedvt)
    fbdev_redrawvt(fbdev_getvt());

  /* Make sure we init while on the right VT */
  ioctl(ttyfd, VT_WAITACTIVE, fbdev_pgvt);

  /* Now open the right TTY */
  snprintf(buf,sizeof(buf)-1,"/dev/tty%d",fbdev_pgvt);
  buf[sizeof(buf)-1]=0;
  ttyfd = open(buf, O_RDWR);
  if (ttyfd <= 0)
    return mkerror(PG_ERRT_IO,107);   /* can't open TTY */

  return success;
}

#endif /* CONFIG_FB_VT */
/**************************************** Framebuffer initalization */

g_error fbdev_init(void) {
#ifdef CONFIG_FB_VT
   g_error e;
#endif
#ifdef CONFIG_FB_YUV16
   int yuv_bpp;
#endif
   /* Open the framebuffer device */
   if (!(fbdev_fd = open(get_param_str("video-fbdev","device",DEFAULT_FB), O_RDWR)))
     return mkerror(PG_ERRT_IO,95);        /* Error opening framebuffer */

#ifdef CONFIG_FB_VT
   /* Init the whole VT mess */
   e = fbdev_initvt();
   errorcheck;
#else
   /* Just open the TTY specified in the config file */
   ttyfd = open(get_param_str("video-fbdev","ttydev",DEFAULT_TTY), O_RDWR);
#endif
   
   /* Get info on the framebuffer */
   if ((ioctl(fbdev_fd,FBIOGET_FSCREENINFO,&fixinfo) < 0) ||
       (ioctl(fbdev_fd,FBIOGET_VSCREENINFO,&varinfo) < 0))
     return mkerror(PG_ERRT_IO,97);        /* Framebuffer ioctl error */
   
   FB_BPL = fixinfo.line_length;
   vid->xres   = varinfo.xres;
   vid->yres   = varinfo.yres;
   vid->bpp    = varinfo.bits_per_pixel;

#ifdef CONFIG_FB_YUV16
   yuv16_y_seg_size  = vid->xres * vid->yres;
   yuv16_y_seg_pitch = vid->xres;
   yuv_bpp = vid->bpp;
   if(yuv_bpp == 16) vid->bpp = 32; /* so that we have 32bpp offscreen bmaps */
#endif

   /* Load a VBL */
   switch (FB_TYPE(fixinfo.type,vid->bpp)) {
      
#ifdef CONFIG_VBL_LINEAR1
    case 1:
      setvbl_linear1(vid);
      break;
#endif
      
#ifdef CONFIG_VBL_LINEAR2
    case 2:
      setvbl_linear2(vid);
      break;
#endif
      
#ifdef CONFIG_VBL_LINEAR4
    case 4:
      setvbl_linear4(vid);
      break;
#endif
      
#ifdef CONFIG_VBL_LINEAR8
    case 8:
      setvbl_linear8(vid);
      break;
#endif
      
#ifdef CONFIG_VBL_LINEAR16
    case 12:
    case 15:
    case 16:
      setvbl_linear16(vid);
      break;
#endif

#ifdef CONFIG_VBL_LINEAR24
    case 24:
      setvbl_linear24(vid);
      break;
#endif

#ifdef CONFIG_VBL_LINEAR32
    case 32:
# ifdef CONFIG_FB_YUV16
      if(yuv_bpp==16) setvbl_default(vid);
      else            setvbl_linear32(vid);
# else
	setvbl_linear32(vid);
# endif
      break;
#endif

#ifdef CONFIG_VBL_VGAPLAN4
    case FB_TYPE(FB_TYPE_VGA_PLANES,4):
      setvbl_vgaplan4(vid);
      break;
#endif

    default:
      close(fbdev_fd);
      return mkerror(PG_ERRT_BADPARAM,101);   /* Unknown bpp */
   }

   /* Optionally enable the slowvbl for debugging 
    */
#ifdef CONFIG_VBL_SLOWVBL
   if (get_param_int("video-fbdev","slowvbl",0))
     setvbl_slowvbl(vid);
#endif

   /* There are several encodings that can be used for 16bpp true
    * color, including 5-6-5 and 4-4-4 color. These color conversion
    * functions use the info in the 'varinfo' structure to handle
    * any encoding.
    */
   if (vid->bpp > 8) {
     vid->color_hwrtopg = &fbdev_color_hwrtopg;
     vid->color_pgtohwr = &fbdev_color_pgtohwr;
   }

#ifdef CONFIG_FB_YUV16
   if (yuv_bpp == 16) {
     vid->color_hwrtopg = &yuv16_color_hwrtopg;
     vid->color_pgtohwr = &yuv16_color_pgtohwr;
     vid->pixel         = &fbdev_yuv16_pixel;
     vid->getpixel      = &fbdev_yuv16_getpixel;
   }
#endif

   /* Map it */
   FB_MEM = mmap(0,fbdev_mapsize = fixinfo.smem_len,
		      PROT_READ|PROT_WRITE,MAP_SHARED,fbdev_fd,0);
   if (FB_MEM == MAP_FAILED) {
      close(fbdev_fd);
      return mkerror(PG_ERRT_IO,96);       /* Error mapping framebuffer */
   }
   
#ifdef CONFIG_FB_PSION
   /* Not sure why this is necessary on the Psion yet.. */
   FB_MEM += 32;
#endif

   /* Put the console into graphics-only mode */
#ifndef CONFIG_FB_NOGRAPHICS
   ioctl(ttyfd, KDSETMODE, KD_GRAPHICS);
#endif

   /* Save original palette */
   {
      struct fb_cmap colors;
      colors.start = 0;
      colors.len = 16;
      colors.red = fbdev_saved_r;
      colors.green = fbdev_saved_g;
      colors.blue = fbdev_saved_b;
      colors.transp = NULL;
      ioctl(fbdev_fd, FBIOGETCMAP, &colors);
   }
   
   /* Set up a palette for RGB simulation */
   if (vid->bpp == 8) {
      struct fb_cmap colors;
      u16 reds[256],greens[256],blues[256];
      int i;
      
      colors.start  = 0;
      colors.len    = 256;
      colors.red    = reds;
      colors.green  = greens;
      colors.blue   = blues;
      colors.transp = NULL;
      
      for (i=0;i<256;i++) { 
	reds[i]   = (((u32)i) & 0xC0) * 0xFFFF / 0xC0;
	greens[i] = (((u32)i) & 0x38) * 0xFFFF / 0x38;
	blues[i]  = (((u32)i) & 0x07) * 0xFFFF / 0x07;
      }
      
      ioctl(fbdev_fd,FBIOPUTCMAP,&colors);
   }

#ifdef CONFIG_FIX_VR3
   // Fix the screwed up color palette on the VR3
   {
     u16 red[16], green[16], blue[16];
     struct fb_cmap cmap;
     int i;
     
     for(i = 0; i < 16; i++) {
       red[i] = vr_lcd_intensity[i];
       green[i] = vr_lcd_intensity[i];
       blue[i] = vr_lcd_intensity[i];
     }
     
     cmap.start = 0;
     cmap.len = 16;
     cmap.red = red;
     cmap.green = green;
     cmap.blue = blue;
     cmap.transp = 0;
     ioctl(fbdev_fd, FBIOPUTCMAP, &cmap);
   } 
#endif
   
#ifdef CONFIG_FB_VT
   {
     struct vt_mode mode;

     /* Get the kernel to bug us about VT changes */
     ioctl(ttyfd, VT_GETMODE, &mode);
     mode.mode = VT_PROCESS;
     mode.relsig = SIGVT;
     mode.acqsig = SIGVT;
     ioctl(ttyfd, VT_SETMODE, &mode);
     
     /* Ready to process VT switches! */
     fbdev_handler_on = 1;
   }
#endif /* CONFIG_FB_VT */

   /* Optionally set up double-buffering */
   if (get_param_int("video-fbdev","doublebuffer",0)) {
     g_error e;
     
     screen_buffer = vid->display;
     ((struct stdbitmap *)screen_buffer)->w     = vid->xres;
     ((struct stdbitmap *)screen_buffer)->h     = vid->yres;
     ((struct stdbitmap *)screen_buffer)->bpp   = vid->bpp;
     ((struct stdbitmap *)screen_buffer)->pitch = fixinfo.line_length;

#ifdef CONFIG_FB_PAGEFLIP
     /* If we're page flipping, create the backbuffer in VRAM
      */
     e = g_malloc((void**)&vid->display, sizeof(struct stdbitmap));
     errorcheck;
     memset(vid->display,0,sizeof(struct stdbitmap));
     ((struct stdbitmap *)vid->display)->w     = vid->xres;
     ((struct stdbitmap *)vid->display)->h     = vid->yres;
     ((struct stdbitmap *)vid->display)->bpp   = vid->bpp;
     ((struct stdbitmap *)vid->display)->pitch = fixinfo.line_length;
     ((struct stdbitmap *)vid->display)->bits  = 
       ((struct stdbitmap *)screen_buffer)->bits + fixinfo.line_length * vid->yres;
     fbdev_flipped = 0;

     /* And set up the virtual resolution for 2 buffers stacked vertically
      */
     varinfo.xres_virtual = varinfo.xres;
     varinfo.yres_virtual = varinfo.yres << 1;
     varinfo.xoffset = 0;
     varinfo.yoffset = 0;
     ioctl(fbdev_fd,FBIOPUT_VSCREENINFO,&varinfo);  
#else
     /* Otherwise create it in system memory
      */
     e = VID(bitmap_new)(&vid->display, vid->xres, vid->yres, vid->bpp);
     errorcheck;
#endif

     vid->update = fbdev_doublebuffer_update;
   }
   else {
     screen_buffer = NULL;
     vid->update = def_update;
   }

#ifdef CONFIG_FB_YUV16
   if(yuv_bpp == 16) {
     yuv_rgb_shadow_buffer = malloc(vid->xres * vid->yres * sizeof(hwrcolor));
     if(yuv_rgb_shadow_buffer==0) {
       fbdev_close();
       return mkerror(PG_ERRT_MEMORY, 25); /* No mem for RGB shadow buffer */
     }
   }
#endif

   return success;
}

void fbdev_close(void) {

#ifdef CONFIG_FB_YUV16
  /* free RGB shadow buffer */
  if(yuv_rgb_shadow_buffer) free(yuv_rgb_shadow_buffer);
#endif

  /* If the page is flipped, flip it back to normal */
#ifdef CONFIG_FB_PAGEFLIP
  if (fbdev_flipped) {
    hwrbitmap tmp = vid->display;
    vid->display = screen_buffer;
    screen_buffer = tmp;
  }
#endif

   /* Shut down double-buffer */
   if (screen_buffer) {
#ifdef CONFIG_FB_PAGEFLIP
     /* The page flipping buffer is in VRAM, so we
      * only need to free a stdbitmap structure
      */
     g_free(vid->display);
#else
     VID(bitmap_free)(vid->display);
#endif
     vid->display = screen_buffer;
     screen_buffer = NULL;
     vid->update = def_update;
   }

   /* Clear the screen before leaving */
   VID(rect)(vid->display,0,0,vid->lxres,vid->lyres,0,PG_LGOP_NONE);

   /* Restore original palette */
   {
      struct fb_cmap colors;
      colors.start = 0;
      colors.len = 16;
      colors.red = fbdev_saved_r;
      colors.green = fbdev_saved_g;
      colors.blue = fbdev_saved_b;
      colors.transp = NULL;
      ioctl(fbdev_fd, FBIOPUTCMAP, &colors);
   }

#ifdef CONFIG_FB_PSION
   /* Not sure why this is necessary on the Psion yet.. */
   FB_MEM -= 32;
#endif

   munmap(FB_MEM,fbdev_mapsize);
   
   /* Back to text mode */
#ifndef CONFIG_FB_NOGRAPHICS
   ioctl(ttyfd, KDSETMODE, KD_TEXT);
#endif

#ifdef CONFIG_FB_VT
   { 
     struct vt_mode mode;
     
     /* Use automatic vt changes again
      */
     fbdev_handler_on = 0;
     ioctl(ttyfd, VT_GETMODE, &mode);
     mode.mode = VT_AUTO;
     mode.relsig = 0;
     mode.acqsig = 0;
     ioctl(ttyfd, VT_SETMODE, &mode);

     if (fbdev_getvt() == fbdev_savedvt) {
       /* Refresh the text mode */
       fbdev_redrawvt(fbdev_getvt());
     }
     else {
       /* Back to the original console */
       ioctl(ttyfd, VT_ACTIVATE, fbdev_savedvt);
     }
   }
#endif

   close(fbdev_fd);
   close(ttyfd);
}

void fbdev_doublebuffer_update(s16 x,s16 y,s16 w,s16 h) {
#ifdef CONFIG_FB_SYNC
  /* My first attempt at a VBL-sync'ed framebuffer...
   * IMHO this method sucks so hopefully there's a better way.
   */
  struct fb_vblank vb;
  do {
    ioctl(fbdev_fd, FBIOGET_VBLANK, &vb);
  } while ((vb.flags & FB_VBLANK_HAVE_VSYNC) && !(vb.flags & FB_VBLANK_VSYNCING));
#endif

#ifdef CONFIG_FB_PAGEFLIP
  /* Flip the buffers, then blit changed areas back to the old buffer */

  if (fbdev_flipped) {
    fbdev_flipped = 0;
    varinfo.yoffset = -vid->yres;
  }
  else {
    fbdev_flipped = 1;
    varinfo.yoffset = vid->yres;
  }
  {
    hwrbitmap tmp = vid->display;
    vid->display = screen_buffer;
    screen_buffer = tmp;
  }
  ioctl(fbdev_fd,FBIOPAN_DISPLAY,&varinfo);
  vid->blit(vid->display, x,y,w,h, screen_buffer, x,y, PG_LGOP_NONE);

#else
  /* Just blit the changed areas */
  vid->blit(screen_buffer, x,y,w,h, vid->display, x,y, PG_LGOP_NONE);
#endif
}

g_error fbdev_regfunc(struct vidlib *v) {
   v->init = &fbdev_init;
   v->close = &fbdev_close;

#ifdef CONFIG_FB_VT
   v->message = &fbdev_message;
#endif

   return success;
}

/* The End */
