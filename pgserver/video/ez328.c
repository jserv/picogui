/* $Id: ez328.c,v 1.7 2001/02/23 17:46:42 pney Exp $
 *
 * ez328.c - Driver for the 68EZ328's (aka Motorola Dragonball EZ)
 *           built-in LCD controller. It assumes the LCD parameters
 *           have been set at boot in the kernel (for the boot logo)
 *           and uses them to set up the driver.
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

#include <pgserver/common.h>

#include <pgserver/video.h>
#include <pgserver/input.h>
#include <asm/MC68EZ328.h>   /* Defines the CPU and peripheral's registers */

/* Save all LCD registers, restore on exit */
#define REGS_LEN     0x36
#define REGS_START   ((void*)LSSA_ADDR)
unsigned char *ez328_saveregs[REGS_LEN];

g_error ez328_init(int xres,int yres,int bpp,unsigned long flags) {
   g_error e;
   
#ifdef CONFIG_CHIPSLICE
   LCKCON = 0;     /* LCKCON - LCD is off */
   LVPW   = 0x50;
   LXMAX  = 0x140;
   LYMAX  = 0x0EF;  /* because of a 'LYMAX+1' somewhere */
   LRRA   = 0;
   LPXCD  = 0;
   LPICF  = 0x0A;   /* =1010 = 16 grey 4 bits */
   LPOLCF = 0x00;

   LCKCON = 0x81;
   PCPDEN = 0xff00;     /* LCD pins */
#endif

   /* Save existing register settings */
   memcpy(ez328_saveregs,REGS_START,REGS_LEN);
   
   vid->xres   = LXMAX;
   vid->yres   = LYMAX+1;
   vid->bpp    = 4;
   vid->fb_bpl = (vid->xres * vid->bpp) >> 3;
   
   /* Allocate video memory */
   e = g_malloc((void **) &vid->fb_mem, vid->yres * vid->fb_bpl);
   errorcheck;
   LSSA = (unsigned long) vid->fb_mem;

#ifdef CONFIG_CHIPSLICE
   /* Load the ts driver as the main input driver */
   return load_inlib(&tsinput_regfunc,&inlib_main);
#else
   return sucess;
#endif
}

void ez328_close(void) {
   /* Restore register settings, free video memory */
   memcpy(REGS_START,ez328_saveregs,REGS_LEN);   
   g_free(vid->fb_mem);
}

g_error ez328_regfunc(struct vidlib *v) {
   setvbl_linear4(v);
   v->init = &ez328_init;
   v->close = &ez328_close;
   return sucess;
}

/* The End */
