/* $Id: fbdev.c,v 1.1 2001/01/20 06:59:09 micahjd Exp $
 *
 * fbdev.c - Some glue to use the linear VBLs on /dev/fb*
 * 
 * This driver supports 8,16,24, and 32 bit color at any resolution.
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

#ifdef DRIVER_FBDEV

#include <pgserver/video.h>

#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <linux/fb.h>

/* This information is only saved so we can munmap() and close()... */
int fbdev_fd;
unsigned long fbdev_mapsize;

g_error fbdev_init(int xres,int yres,int bpp,unsigned long flags) {
   g_error e;
   int fbdev_fd;
   struct fb_fix_screeninfo fixinfo;
   struct fb_var_screeninfo varinfo;
   
   /* Open the framebuffer device */
   if (!(fbdev_fd = open("/dev/fb0", O_RDWR)))
     return mkerror(PG_ERRT_IO,95);        /* Error opening framebuffer */
   
   /* Get info on the framebuffer */
   if ((ioctl(fbdev_fd,FBIOGET_FSCREENINFO,&fixinfo) < 0) ||
       (ioctl(fbdev_fd,FBIOGET_VSCREENINFO,&varinfo) < 0))
     return mkerror(PG_ERRT_IO,97);        /* Framebuffer ioctl error */
   
   vid->fb_bpl = fixinfo.line_length;
   vid->xres   = varinfo.xres;
   vid->yres   = varinfo.yres;
   vid->bpp    = varinfo.bits_per_pixel;
   
   /* Map it */
   vid->fb_mem = mmap(0,fbdev_mapsize = fixinfo.smem_len,
		      PROT_READ|PROT_WRITE,MAP_SHARED,fbdev_fd,0);
   if (vid->fb_mem == MAP_FAILED) {
      close(fbdev_fd);
      return mkerror(PG_ERRT_IO,96);       /* Error mapping framebuffer */
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
   
   return sucess;
}

void fbdev_close(void) {
   munmap(vid->fb_mem,fbdev_mapsize);
   close(fbdev_fd);
}

g_error fbdev_regfunc(struct vidlib *v) {
   setvbl_linear8(v);           /* Only 8bpp so far */
   v->init = &fbdev_init;
   v->close = &fbdev_close;
   return sucess;
}

#endif /* DRIVER_FBDEV */
/* The End */
