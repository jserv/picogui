/* $Id: ez328.c,v 1.1 2001/01/29 00:22:34 micahjd Exp $
 *
 * ez328.c - Driver for the 68EZ328's (aka Motorola Dragonball EZ)
 *           built-in LCD controller. It assumes the LCD parameters
 *           have been set at boot in the kernel (for the boot logo)
 *           and uses them to set up the driver.
 * 
 * NOTE: very slow and kludgey now. Just need to see if it will work.
 *       currently hard-coded for Kiwi's LCD.
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

#ifdef DRIVER_EZ328

#include <pgserver/video.h>
#include <asm/MC68EZ328.h>   /* Defines the CPU and peripheral's registers */

/* Saved video memory, so we restore the boot logo when done */
char *ez328_oldfb;

g_error ez328_init(int xres,int yres,int bpp,unsigned long flags) {
   g_error e;
   
   /* Get existing settings */
   ez328_oldfb = LSSA;
/*
   vid->xres   = 240;
   vid->yres   = 32;
   vid->fb_bpl = 240;
   vid->bpp    = 8;
  */
   vid->xres = 240;
   vid->yres = 64;
   vid->bpp = 8;
   
   /* Allocate more video memory */
   e = g_malloc((void **) &vid->fb_mem, 240*32+1);
   errorcheck;
   /* We use the above margin of 1 byte to align if necessary.
    * the video memory must start on a word boundary. */
   if (((unsigned long)vid->fb_mem) & 1)
     vid->fb_mem++;
   LSSA = vid->fb_mem;
   
   return sucess;
}

void ez328_close(void) {
   /* Restore previous image */
   LSSA = ez328_oldfb;
}

void ez328_pixel(int x,int y,hwrcolor c) {
/*   if (x<0 || y<0 || x>=240 || y>=64) {
      printf("Bad pixel coordinates (%d,%d) (c=0x%08X)\n",x,y,c);
      return;
   }*/
   
  if (y<32) {
    vid->fb_mem[y*240+x] &= 0xF0;
    vid->fb_mem[y*240+x] |= c;
  }
  else {
    vid->fb_mem[(y-32)*240+x] &= 0x0F;
    vid->fb_mem[(y-32)*240+x] |= c << 4;
  }
}

hwrcolor ez328_getpixel(int x,int y) {
   if (x<0 || y<0 || x>=240 || y>=64) {
      printf("Bad pixel coordinates (%d,%d) in getpixel)\n",x,y);
      return;
   }
   
   if (y<32) {
      return vid->fb_mem[y*240+x] & 0x0F;
   }
   else {
      return vid->fb_mem[(y-32)*240+x] >> 4;
   }
}

hwrcolor ez328_color_pgtohwr(pgcolor c) {
   return 15 - (getred(c)+getgreen(c)+getblue(c))/48;
}
   
g_error ez328_regfunc(struct vidlib *v) {
//   setvbl_linear8(v);
   setvbl_default(v);
   
   v->init = &ez328_init;
   v->close = &ez328_close;
   v->pixel = &ez328_pixel;
   v->getpixel = &ez328_getpixel;
   v->color_pgtohwr = &ez328_color_pgtohwr;
   return sucess;
}

#endif /* DRIVER_EZ328 */
/* The End */
