/* $Id: ez328vga.c,v 1.1 2001/06/29 07:50:59 micahjd Exp $
 *
 * ez328vga.c - Another of my strange drivers...
 *              Using the 68EZ328's LCD controller and a few logic gates,
 *              this driver can display 8-color video on a VGA monitor.
 *              This has been tested/calibrated on a 68EZ328 with a 32.768mhz
 *              crystal. (the uCsimm)
 *              It should work even better on a 'vz328, but the timing will
 *              need to be changed.
 *              Currently the resolution is 120x480. It uses 640x480 timing,
 *              but I can't seem to get this thing to make a pixel clock higher
 *              than 4mhz. If anyone has any ideas please let me know.
 *              Please excuse the ASCII art, but below is the schematic for
 *              connecting the 'ez328 to a VGA monitor.
 * 
 *              IMPORTANT NOTE: Because the sync signals are generated in
 *              software, it is entirely possible for a corrupted framebuffer
 *              or even a color greater than 7 to damage the monitor!
 *              It is not my fault if building this project damages you,
 *              your monitor, your uCsimm, or anything else.
 *              Even though the linear4 VBL would work fine for this
 *              driver, for now it is not used. There are two reasons for this:
 *
 *                - Safety. Only implementing the low-level functions lets
 *                  us do more error checking on the color values
 *
 *                - Line repetition. The current implementation has very
 *                  non-square pixels. This driver draws a vertical line of
 *                  pixels for every pixel() drawn to make it look a little
 *                  better.
 * 
 * -------------------------------8<---------------------------------
 * 
 *    'ez328                                                       VGA
 *   .-------.                 AND gate (1/4 74HCT08)              .-------.
 *   |       |                         .----.                      |       |
 *   |  LD0  |-----+-------------------|     \         68 ohms     |       |
 *   |       |     |                   |      )--------/\/\/\/-----| Blue  |
 *   |       |     |                .--|     /                     |       |
 *   |       |     |                |  `----'                      |       |
 *   |       |     |                |  .----.                      |       |
 *   |  LD1  |--+----------------------|     \         68 ohms     |       |
 *   |       |  |  |                |  |      )--------/\/\/\/-----| Green |
 *   |       |  |  |                +--|     /                     |       |
 *   |       |  |  |                |  `----'                      |       |
 *   |       |  |  |                |  .----.                      |       |
 *   |  LD2  |-------------------------|     \         68 ohms     |       |
 *   |       |  |  |                |  |      )--------/\/\/\/-----| Red   |
 *   |       |  |  |                +--|     /                     |       |
 *   |       |  |  |                |  `----'                      |       |
 *   |       |  |  |     .----.     |                              |       |
 *   |       |  |  |  .--|     \    |                              |       |
 *   |  LD3  |--------+  |      )O--'                              |       |
 *   |       |  |  |  +--|     /     (NAND gate used as inverter)  |       |
 *   `-------'  |  |  |  `----'                                    |       |  
 *              |  |  |                                            |       |
 *              |  |  |  .----.                                    |       |
 *              |  |  +--|     \                                   |       |
 *              |  |  |  |      )O---------------------------------| Hsync |
 *              |  `--|--|     /                                   |       |
 *              |     |  `----'                                    |       |
 *              |     |                                            |       |
 *              |     |  .----.                                    |       |
 *              |     `--|     \                                   |       |
 *              |        |      )O---------------------------------| Vsync |
 *              `--------|     /                                   |       |
 *                       `----'                                    |       |
 *                       NAND gate (1/4 74HCT00)                   `-------' 
 * 
 * Explanation:
 * 
 *   The LCD controller is programmed for a 4-bit data bus. When LD3 is zero,
 *   LD0-LD2 pass through the circuit to form the RGB color, and both sync
 *   signals are high. To generate a sync pulse, LD3 is set to one. The RGB
 *   color is now 000 and LD0 forms the (inverted) Hsync signal and LD1 forms
 *   the (inverted) Vsync signal.
 * 
 *   The 68 ohm resistor forms a voltage divider with the monitor's 75 ohm
 *   impedence to give an output voltage of about 0.7 volts (Standard VGA)
 * 
 *   A framebuffer larger than the visible area of the screen is used so the
 *   sync signals are 'drawn' on the right and bottom sides of the framebuffer
 *   with the visible image occupying the top-left corner.
 *
 * Known problems:
 * 
 *   The driver is very inefficient.
 *   This isn't really good for anything useful in its current stage. The
 *   resolution is much too low. Also, there is a horizontal 'jitter' on the
 *   screen that I think is caused by the 'ez328's DMA process. Maybe a latch
 *   on the output would cure this?
 *   Perhaps with some experimentation this could be turned into a useful
 *   video output method for the 'ez328 and PicoGUI.
 *
 * -------------------------------8<---------------------------------
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

/* This is for a 68EZ328 with a 32.768khz crystal */
 
#define PHYSWIDTH   140   /* Must be <= 255 and divisible by 4 */
#define VIRTWIDTH   120
#define HSYNCSTART  65    /* In bytes */
#define HSYNCWIDTH  2     /* In bytes */
#define PHYSLINES   512
#define VIRTLINES   480
#define PGUILINES   120   /* Resolution after line repetition */
#define PGUIREPEAT  4     /* Amount of line repetition */
#define VSYNCSTART  494   /* In lines */
#define VSYNCWIDTH  378   /* In bytes */
#define FBSIZE      ((PHYSWIDTH*PHYSLINES)>>1)
#define FBBPL       (PHYSWIDTH>>1)
    
unsigned char *ez328vga_fb;

g_error ez328vga_init(void) {
   g_error e;
   int i,j;
   unsigned char *p;
   
   vid->display = NULL;
   vid->xres    = VIRTWIDTH;
   vid->yres    = PGUILINES;
   vid->bpp     = 4;
   
   e = g_malloc((void**) &ez328vga_fb,FBSIZE);
   errorcheck;
   
   /* Init a PHYSWIDTH * PHYSHEIGHT mode with a 4bit bus
    * in black&white mode */
   
   *((unsigned char **)0xfffffa00) = ez328vga_fb;
   *((unsigned char *)0xfffffa05)  = PHYSWIDTH / 4;
   *((unsigned short *)0xfffffa08) = PHYSWIDTH * 4;
   *((unsigned short *)0xfffffa0a) = PHYSLINES - 1;
   *((unsigned char *)0xfffffa29)  = 0x00;
   *((unsigned char *)0xfffffa20)  = 0x08;
   *((unsigned char *)0xfffffa27)  = 0x81;
   *((unsigned char *)0xfffffa25)  = 0x00;
   *((unsigned short *)0xfffff412) = 0xff00;
   *((unsigned char *)0xfffffa21)  = 0x00;
   
   /* Clear memory, then 'draw' the horizontal and vertical sync signals 
    * (see above schematic explanation) */
   
   memset(ez328vga_fb,0,FBSIZE);
   for (i=PHYSLINES,p=ez328vga_fb+HSYNCSTART;i;i--,p+=FBBPL)
     memset(p,0x99,HSYNCWIDTH);
   p = ez328vga_fb + VSYNCSTART*FBBPL;
   for (i=VSYNCWIDTH;i;i--,p++) {
      if (*p == 0x99)
	*p = 0xBB;
      else
	*p = 0xAA;
   }
   
   return sucess;
}


void ez328vga_close(void) {
   g_free(ez328vga_fb);
}

void ez328vga_pixel(hwrbitmap dest,s16 x,s16 y,hwrcolor c,s16 lgop) {
   unsigned char *p;
   int i;
   
   if (dest || lgop!=PG_LGOP_NONE) {
      def_pixel(dest,x,y,c,lgop);
      return;
   }

   if (x<0 || y<0 || (x>=vid->xres) || (y>=vid->yres))
     return;
   
   c &= 7;   /* important! */
   
   p = ez328vga_fb + (x>>1) + y*FBBPL*PGUIREPEAT;
   for (i=PGUIREPEAT;i;i--,p+=FBBPL) {
      if (x&1) {
	 *p &= 0xF0;
	 *p |= c;
      }
      else {
	 *p &= 0x0F;
	 *p |= c<<4;
      }
   }
}

hwrcolor ez328vga_getpixel(hwrbitmap src,s16 x,s16 y) {
   unsigned char *p;
   
   if (src)
     return def_getpixel(src,x,y);

   p = ez328vga_fb + (x>>1) + y*FBBPL*PGUIREPEAT;
   if (x&1)
     return (*p) & 7;
   return (*p) >> 4;
}

hwrcolor ez328vga_color_pgtohwr(pgcolor c) {
   return ((getred(c)&0x80) >> 5) |
          ((getgreen(c)&0x80) >> 6) |
          ((getblue(c)&0x80) >> 7);
}

pgcolor ez328vga_color_hwrtopg(hwrcolor c) {
   return ( (c&4) ? 0xFF0000 :0) |
          ( (c&2) ? 0x00FF00 :0) |
          ( (c&1) ? 0x0000FF :0);
}
   
g_error ez328vga_regfunc(struct vidlib *v) {
   setvbl_default(v);
   
   v->init     = &ez328vga_init;
   v->color_hwrtopg = &ez328vga_color_hwrtopg;
   v->color_pgtohwr = &ez328vga_color_pgtohwr;   
   v->close    = &ez328vga_close;
   v->pixel    = &ez328vga_pixel;
   v->getpixel = &ez328vga_getpixel;
   return sucess;
}

/* The End */
