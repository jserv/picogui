/* $Id$
 *
 * serial40x4.c - PicoGUI video driver for a serial wall-mounted
 *                40x4 character LCD I put together about a year ago.
 *                This driver is very similar to the ncurses driver, but
 *                includes lower level support for this LCD. All LCD commands
 *                are sent to /dev/lcd, which can be a symbolic link to a
 *                serial port (already in the right configuration) that the
 *                LCD is attached to.
 * 
 * I apologize in advance for all the hardcoded font data, but the font
 * compiler isn't quite suitable for this lcd (and there's really only
 * two fonts you'd ever need for this thing anyway, right?)
 * 
 * There's no need to compile any fonts into pgserver, this driver includes
 * two of its own. A normal 1-to-1 mapping used by default, and a 4-line-high
 * large font. Currently, you can invoke this large font by requesting a
 * font at least 20 pixels high in pgNewFont. This may change.
 * 
 * ------ Data format used by the serial LCD
 * 
 * Serial data at 9600 baud, 8-N-1. Unless otherwise specified, a character
 * sent to the LCD is inserted at the cursor location, and the cursor location
 * is advanced. The 40x4 LCD is made up of two 40x2 HD44780-compatible
 * LCD controllers, each with its own parameters and custom characters
 * independant of the other. The display recognizes the following escape codes:
 * 
 *  \\    Insert a '\' character
 *  \1    Switch to first (top) LCD controller
 *  \2    Switch to second (bottom) LCD controller
 *  \*    Send commands to all LCD controllers simult-aneously
 *  \c?   Sends a one-character LCD command, see HD44780 docs
 *  \s    Clear screen
 *  \g    Put cursor at the beginning of character-generator RAM
 *  \d    Put cursor at the beginning of display RAM
 *  \b    Beep the LCD unit's speaker
 *  \f    Flash the LCD unit's LED
 *  \n    Go to the second line of the selected LCD controller
 * 
 * The LCD unit's firmware is a C program running on a PIC16C84
 * microcontroller. It is designed to be compiled using the 'c2c' compiler
 * available for Linux.
 *
 * Source code is included in the file serial40x4.firmware.c, released to the
 * public domain.
 * 
 * ------
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2003 Micah Dowty <micahjd@users.sourceforge.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 * 
 * Contributors:
 * 
 * 
 * 
 */

#include <pgserver/common.h>

#include <pgserver/video.h>
#include <pgserver/input.h>
#include <pgserver/appmgr.h>
#include <pgserver/font.h>
#include <pgserver/render.h>
#include <pgserver/timer.h>
#include <pgserver/configfile.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/* Macros to easily access the members of vid->display */
#define FB_MEM   (((struct stdbitmap*)vid->display)->bits)
#define FB_BPL   (((struct stdbitmap*)vid->display)->pitch)

/* A shadow buffer so we can see what has changed */
u8 *lcd_shadow;

int lcd_fd;

/******************************************** Fake font */
/* This is a little hack to trick PicoGUI's text rendering */

/* Glyph table for the bigfont */
struct fontglyph const serial40x4_bigfont_glyphs[] = {
  { ' ', 0x0000, 2, 0,0,0,0 },
  { '0', 0x0000, 4, 3,4,0,0 },
  { '1', 0x000C, 2, 1,4,0,0 },
  { '2', 0x0010, 4, 3,4,0,0 },
  { '3', 0x001C, 4, 3,4,0,0 },
  { '4', 0x0028, 4, 3,4,0,0 },
  { '5', 0x0034, 4, 3,4,0,0 },
  { '6', 0x0040, 4, 3,4,0,0 },
  { '7', 0x004C, 4, 3,4,0,0 },
  { '8', 0x0058, 4, 3,4,0,0 },
  { '9', 0x0064, 4, 3,4,0,0 },
  { ':', 0x0070, 2, 1,4,0,0 },
};

/* Font bitmaps for the bigfont */
#define S ' '
u8 const serial40x4_bigfont_bitmaps[] = {
   3,2,4,0,S,0,0,S,0,3,1,4,        /* 0x0000 - 0     */
   3,0,0,1,                        /* 0x000C - 1     */   
   3,2,4,3,2,5,0,S,S,1,1,1,        /* 0x0010 - 2     */   
   3,2,4,S,3,5,S,3,5,3,1,4,        /* 0x001C - 3     */   
   3,S,3,0,S,0,3,1,0,S,S,1,        /* 0x0028 - 4     */   
   2,2,4,0,2,4,S,S,0,3,1,4,        /* 0x0034 - 5     */   
   3,2,4,0,2,4,0,S,0,3,1,4,        /* 0x0040 - 6     */   
   3,2,2,S,3,5,S,0,S,S,1,S,        /* 0x004C - 7     */   
   3,2,4,0,2,0,0,S,0,3,1,4,        /* 0x0058 - 8     */   
   3,2,4,0,S,0,3,1,0,S,S,1,        /* 0x0064 - 9     */   
   S,1,2,S,                        /* 0x0070 - :     */   
};
#undef S

u8 serial40x4_font_bitmap;

struct font const serial40x4_font = {
  numglyphs: 256,
  defaultglyph: ' ',
  w: 1,
  h: 1,
  ascent: 1,
  descent: 0,
  glyphs: NULL,
  bitmaps: &serial40x4_font_bitmap
};
struct font const serial40x4_bigfont = {
  numglyphs: 12,
  defaultglyph: ' ',
  w: 4,
  h: 4,
  ascent: 4,
  descent: 0,
  glyphs: (struct fontglyph const *) &serial40x4_bigfont_glyphs,
  bitmaps: (u8 const *) &serial40x4_bigfont_bitmaps
};

/* Bogus fontstyle nodes */
struct fontstyle_node serial40x4_font_style = {
  name: "LCD Pseudofont",
  size: 1,
  flags: PG_FSTYLE_FIXED,
  next: NULL,
  normal: (struct font *) &serial40x4_font,
  bold: NULL,
  italic: NULL,
  bolditalic: NULL,
  boldw: 0
};
struct fontstyle_node serial40x4_bigfont_style = {
  name: "Large LCD Font",
  size: 4,
  flags: 0,
  next: NULL,
  normal: (struct font *) &serial40x4_bigfont,
  bold: NULL,
  italic: NULL,
  bolditalic: NULL,
  boldw: 0
};

/* Custom characters (needed for large text) */
u8 const cg1[] = {
     0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,
     0x1F,0x1F,0x1F,0x1F,0x1F,0x00,0x00,0x00,
     0x00,0x00,0x00,0x1F,0x1F,0x1F,0x1F,0x1F,
     0x00,0x00,0x00,0x01,0x03,0x07,0x0F,0x1F,
     0x00,0x00,0x00,0x10,0x18,0x1C,0x1E,0x1F,
     0x1F,0x1F,0x1E,0x1E,0x1C,0x1C,0x18,0x10,
     0x55,0xAA,0x55,0xAA,0x55,0xAA,0x55,0xAA,
     0x00,0x00,0x15,0x00,0x00,0x15,0x00,0x00
};
u8 const cg2[] = {
     0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,
     0x1F,0x1F,0x1F,0x1F,0x1F,0x00,0x00,0x00,
     0x00,0x00,0x00,0x1F,0x1F,0x1F,0x1F,0x1F,
     0x1F,0x0F,0x07,0x03,0x01,0x00,0x00,0x00,
     0x1F,0x1E,0x1C,0x18,0x10,0x00,0x00,0x00,
     0x10,0x18,0x1C,0x1C,0x1E,0x1E,0x1F,0x1F,
     0x55,0xAA,0x55,0xAA,0x55,0xAA,0x55,0xAA,
     0x00,0x00,0x15,0x00,0x00,0x15,0x00,0x00
};

/******************************************** Implementations */

void serial40x4_loadfonts(void) {
  /* Download custom characters to the LCD */
  write(lcd_fd,"\\1\\g",4);
  write(lcd_fd,cg1,64);
  write(lcd_fd,"\\2\\g",4);
  write(lcd_fd,cg2,64);
}

g_error serial40x4_init(void) {
   g_error e;

   vid->xres = 40;
   vid->yres = 4;
   vid->bpp  = 8;
   FB_BPL = 64;
   
   /* Allocate our buffer (framebuffer and shadow) */
   e = g_malloc((void**) &FB_MEM,FB_BPL * vid->yres * 2);
   errorcheck;
   lcd_shadow = FB_MEM + FB_BPL * vid->yres;
   memset(FB_MEM,0,FB_BPL * vid->yres * 2);
   
   /* Open LCD device */
   lcd_fd = open(get_param_str("video-serial40x4","device","/dev/lcd"),
		 O_WRONLY);
   if (lcd_fd<=0)
     return mkerror(PG_ERRT_IO,46);
    
   serial40x4_loadfonts();
   
   return success;
}

void serial40x4_close(void) {
   g_free(FB_MEM);   /* And shadow buffer */
   close(lcd_fd);
}

/* Use a shadow buffer to send only changed data to the LCD.
 *
 * Cruftily hardcoded to a split 40x4 Hitachi-compatible LCD
 */
void serial40x4_update(hwrbitmap d,s16 x,s16 y,s16 w,s16 h) {
   int i_lcd,i;
   u8 *oldp,*newp;
   /* Output buffer big enough to hold one frame with worst-case encoding */
   u8 outbuf[2048];
   u8 *outp = outbuf;
   int full_update = 0;
   static u32 last_time = 0;
   u32 now;

   /* To make sure the LCD stays consistent, do a full update if it's been
    * more than 5 seconds since the last one. This ensures that transmission
    * errors don't effect the LCD, and if the LCD is plugged in after starting
    * PicoGUI it will synchronize. Note that this is kind of crufty.. I'll improve the
    * protocol when I rewrite this driver.
    */
   now = getticks();
   if (now > 5000+last_time) {
     full_update = 1;
     last_time = now;
   }

   /* If this is a full update, resend the fonts */
   if (full_update)
     serial40x4_loadfonts();

   /* start comparing the buffers */
   i_lcd = -1;
   oldp = lcd_shadow;
   newp = FB_MEM;
   for (i=0;i<256;oldp++,newp++,i++) {

      /* Only update changed areas, unless it's a full update */
      if (!full_update && *oldp == *newp)
	continue;

      /* Don't bother updating the areas past the right edge of the screen */
      if ((i & 0x3F) >= 40)
	continue;
      
      /* Move the hardware cursor if necessary*/
      if (i_lcd != i) {

	 /* set the proper controller */
	 if (i>=128) {
	    if (i_lcd < 128) {
	       *(outp++) = '\\';
	       *(outp++) = '2';
	    }
	 }
	 else {
	    if (i_lcd<0 || i_lcd>=128) {
	       *(outp++) = '\\';
	       *(outp++) = '1';
	    }
	 }

	 /* set address */
	 *(outp++) = '\\';
	 *(outp++) = 'c';
	 *(outp++) = 0x80 + (i%128);   /* HD44780 DDRAM command */
	 i_lcd = i;
      }
      
      /* Escape any backslashes in the data to be written */
      if (*newp == '\\') {
	*(outp++) = '\\';
	*(outp++) = '\\';
      }
      else
	/* Output character */
	*(outp++) = *newp;

      if (i_lcd!=127)            /* barrier between controllers */
	i_lcd++;
      if (i_lcd==40)             /* Weird line wrapping */
	i_lcd = 64;
      if (i_lcd==168)            /* Weird line wrapping */
	i_lcd = 192;
   }

   /* Write buffer contents */
   write(lcd_fd,outbuf,outp-outbuf);
   
   /* Update the shadow buffer */
   memcpy(lcd_shadow,FB_MEM,256);
}

/**** Hack the normal font rendering a bit so we use regular text */

void serial40x4_charblit(hwrbitmap dest, u8 *chardat,s16 dest_x,
		      s16 dest_y,s16 w,s16 h,s16 lines,s16 angle,
		      hwrcolor c,struct pgquad *clip,s16 lgop) {
  struct stdbitmap chbit;

   if (clip && (dest_x<clip->x1 || dest_y<clip->y1 ||
		dest_x>clip->x2 || dest_y>clip->y2))
     return;   
   
   chbit.bits = chardat;
   chbit.w = chbit.pitch = w;
   chbit.h = h;
   (*vid->blit)(dest,dest_x,dest_y,w,h,(hwrbitmap) &chbit,0,0,PG_LGOP_NONE);
}

void serial40x4_font_newdesc(struct fontdesc *fd, const u8 *name,
			     int size, int flags) {
   fd->margin = 0;
   fd->hline = -1;
   fd->italicw = 0;
   if (size>20)     /* Arbitrary number */
     fd->fs = &serial40x4_bigfont_style;
   else
     fd->fs = &serial40x4_font_style;
   fd->font = (struct font *) fd->fs->normal;
}

struct fontglyph const *serial40x4_font_getglyph(struct fontdesc *fd, int ch) {
  /* Fake glyph and bitmap to return */
  static struct fontglyph fakeglyph;

  if (fd->fs != &serial40x4_font_style)
    return def_font_getglyph(fd,ch);

  fakeglyph.encoding  = ch;
  fakeglyph.bitmap    = 0;
  serial40x4_font_bitmap = ch;
  fakeglyph.dwidth    = 1;
  fakeglyph.w         = 1;
  fakeglyph.h         = 1;
  fakeglyph.x         = 0;
  fakeglyph.y         = 0;
  
  return &fakeglyph;
}

/* Like the ncurses driver, if 0x20000000 is set, it's a raw character code.
 * Otherwise, convert to black-and-white */

hwrcolor serial40x4_color_pgtohwr(pgcolor c) {
   if (c & 0x20000000) {
      /* Normal character:  0x200000CC */
      
      return (c & 0x0000FF);
   }
   else {
      /* Black and white, tweaked for better contrast */
      return ((getred(c)+getgreen(c)+getblue(c)) >= 382) ? ' ' : 0; 
   }
   
}

void serial40x4_message(u32 message, u32 param, u32 *ret) {
   char beep[3] = "\\ ";
   
   if (message != PGDM_SOUNDFX) 
     return;
	  
   if (param == PG_SND_BEEP)
     beep[1] = 'b';
   else if (param == PG_SND_VISUALBELL)
     beep[1] = 'f';
   else
     return;
   
   write(lcd_fd,beep,3);
   sleep(1);
}

/******************************************** Driver registration */

g_error serial40x4_regfunc(struct vidlib *v) {
   setvbl_linear8(v);
   
   v->init = &serial40x4_init;
   v->close = &serial40x4_close;
   v->update = &serial40x4_update;  
   v->color_pgtohwr = &serial40x4_color_pgtohwr;
   v->font_newdesc = &serial40x4_font_newdesc;
   v->charblit = &serial40x4_charblit;
   v->message = &serial40x4_message;
   v->font_getglyph = &serial40x4_font_getglyph;
   
   return success;
}

/* The End */
