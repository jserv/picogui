/* $Id: fbdev.c,v 1.25 2002/01/20 09:56:16 micahjd Exp $
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
unsigned long fbdev_mapsize;
int ttyfd;

/* Save screen info for color conversion */
struct fb_fix_screeninfo fixinfo;
struct fb_var_screeninfo varinfo;

#ifdef CONFIG_FIX_VR3
//Color map for the Agenda VR3
static unsigned short vr_lcd_intensity[16] = {
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

/**************************************** Color conversion */

/* Our own conversion routines for true color
 */
pgcolor fbdev_color_hwrtopg(hwrcolor c) {
  return mkcolor( (u8)((c >> varinfo.red.offset  ) << (8 - varinfo.red.length  )),
		  (u8)((c >> varinfo.green.offset) << (8 - varinfo.green.length)),
		  (u8)((c >> varinfo.blue.offset ) << (8 - varinfo.blue.length )) );

}
hwrcolor fbdev_color_pgtohwr(pgcolor c) {
  return ( (((u32)getred(c))   >> (8 - varinfo.red.length  )) << varinfo.red.offset   ) |
         ( (((u32)getgreen(c)) >> (8 - varinfo.green.length)) << varinfo.green.offset ) |
         ( (((u32)getblue(c))  >> (8 - varinfo.blue.length )) << varinfo.blue.offset  );
}

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
  sprintf(buf,"/dev/tty%d",fbdev_pgvt);
  ttyfd = open(buf, O_RDWR);
  if (ttyfd <= 0)
    return mkerror(PG_ERRT_IO,107);   /* can't open TTY */

  return success;
}

#endif /* CONFIG_FB_VT */
/**************************************** Framebuffer initalization */

g_error fbdev_init(void) {
   g_error e;

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
      setvbl_linear32(vid);
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
   
   /* Map it */
   FB_MEM = mmap(0,fbdev_mapsize = fixinfo.smem_len,
		      PROT_READ|PROT_WRITE,MAP_SHARED,fbdev_fd,0);
   if (FB_MEM == MAP_FAILED) {
      close(fbdev_fd);
      return mkerror(PG_ERRT_IO,96);       /* Error mapping framebuffer */
   }

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
      unsigned short reds[256],greens[256],blues[256];
      int i;
      
      colors.start  = 0;
      colors.len    = 256;
      colors.red    = reds;
      colors.green  = greens;
      colors.blue   = blues;
      colors.transp = NULL;
      
      for (i=0;i<256;i++) { 
	reds[i]   = (((unsigned long)i) & 0xC0) * 0xFFFF / 0xC0;
	greens[i] = (((unsigned long)i) & 0x38) * 0xFFFF / 0x38;
	blues[i]  = (((unsigned long)i) & 0x07) * 0xFFFF / 0x07;
      }
      
      ioctl(fbdev_fd,FBIOPUTCMAP,&colors);
   }

#ifdef CONFIG_FIX_VR3
   // Fix the screwed up color palette on the VR3
   {
     unsigned short red[16], green[16], blue[16];
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

   return success;
}

void fbdev_close(void) {
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

g_error fbdev_regfunc(struct vidlib *v) {
   v->init = &fbdev_init;
   v->close = &fbdev_close;

#ifdef CONFIG_FB_VT
   v->message = &fbdev_message;
#endif

   return success;
}

/* The End */
