/* $Id: ez328.c,v 1.2 2001/02/07 07:28:08 micahjd Exp $
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

/* Save all LCD registers, restore on exit */
#define REGS_LEN     0x36
#define REGS_START   ((void*)LSSA_ADDR)
unsigned char *ez328_saveregs[REGS_LEN];

g_error ez328_init(int xres,int yres,int bpp,unsigned long flags) {
   g_error e;
   
   /* Save existing register settings */
   memcpy(ez328_saveregs,REGS_START,REGS_LEN);
   
   vid->xres   = 240;
   vid->yres   = 64;
   vid->fb_bpl = 240;
   vid->bpp    = bpp;
   
   /* Allocate video memory */
   e = g_malloc((void **) &vid->fb_mem, 240*32);
   errorcheck;
   LSSA = (unsigned long) vid->fb_mem;

   /* Invert the palette so that the and/or stuff
    * works and the palette is consistant */
   LPOLCF |= 1;
   
   return sucess;
}

void ez328_close(void) {
   /* Restore register settings, free video memory */
   memcpy(REGS_START,ez328_saveregs,REGS_LEN);   
   g_free(vid->fb_mem);
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
   return (getred(c)+getgreen(c)+getblue(c))/48;
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
