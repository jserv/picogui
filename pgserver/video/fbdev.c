/* $Id: fbdev.c,v 1.20 2002/01/17 07:58:54 micahjd Exp $
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
 * 
 * 
 */

#include <pgserver/common.h>

#include <pgserver/video.h>

#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <linux/vt.h>
#include <linux/kd.h>

/* Macros to easily access the members of vid->display */
#define FB_MEM   (((struct stdbitmap*)vid->display)->bits)
#define FB_BPL   (((struct stdbitmap*)vid->display)->pitch)

/* This information is only saved so we can munmap() and close()... */
int fbdev_fd;
unsigned long fbdev_mapsize;

/* Save screen info for color conversion */
struct fb_fix_screeninfo fixinfo;
struct fb_var_screeninfo varinfo;

pgcolor fbdev_color_hwrtopg(hwrcolor c);
hwrcolor fbdev_color_pgtohwr(pgcolor c);

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

g_error fbdev_init(void) {
   int fbdev_fd;
   
   /* Open the framebuffer device */
   if (!(fbdev_fd = open(get_param_str("video-fbdev","device","/dev/fb0"), O_RDWR)))
     return mkerror(PG_ERRT_IO,95);        /* Error opening framebuffer */
   
   /* Get info on the framebuffer */
   if ((ioctl(fbdev_fd,FBIOGET_FSCREENINFO,&fixinfo) < 0) ||
       (ioctl(fbdev_fd,FBIOGET_VSCREENINFO,&varinfo) < 0))
     return mkerror(PG_ERRT_IO,97);        /* Framebuffer ioctl error */
   
   FB_BPL = fixinfo.line_length;
   vid->xres   = varinfo.xres;
   vid->yres   = varinfo.yres;
   vid->bpp    = varinfo.bits_per_pixel;

   /* Load a VBL */
   switch (vid->bpp) {
      
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

    default:
      close(fbdev_fd);
      return mkerror(PG_ERRT_BADPARAM,101);   /* Unknown bpp */
   }

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
   {
      int xx = open(get_param_str("video-fbdev","ttydev","/dev/tty"), O_RDWR);
      if (xx >= 0) {
	 ioctl(xx, KDSETMODE, KD_GRAPHICS);
	 close(xx);
      }
   }
#endif
   
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
   
   return success;
}

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

void fbdev_close(void) {
   /* Clear the screen before leaving */
   VID(rect)(vid->display,0,0,vid->lxres,vid->lyres,0,PG_LGOP_NONE);

   munmap(FB_MEM,fbdev_mapsize);
   close(fbdev_fd);
   
   /* Back to text mode */
#ifndef CONFIG_FB_NOGRAPHICS
   {
      int xx = open(get_param_str("video-fbdev","ttydev","/dev/tty"), O_RDWR);
      if (xx >= 0) {
	 ioctl(xx, KDSETMODE, KD_TEXT);
	 close(xx);
      }
   }
#endif
}

g_error fbdev_regfunc(struct vidlib *v) {
   v->init = &fbdev_init;
   v->close = &fbdev_close;
   return success;
}

/* The End */
