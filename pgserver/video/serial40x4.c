/* $Id: serial40x4.c,v 1.9 2001/09/02 20:49:25 micahjd Exp $
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
#include <pgserver/appmgr.h>
#include <pgserver/font.h>
#include <pgserver/render.h>

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

struct fontglyph const serial40x4_font_glyphs[] = {
  {1,1,1,0,0,0x00},{1,1,1,0,0,0x01},{1,1,1,0,0,0x02},{1,1,1,0,0,0x03},
  {1,1,1,0,0,0x04},{1,1,1,0,0,0x05},{1,1,1,0,0,0x06},{1,1,1,0,0,0x07},
  {1,1,1,0,0,0x08},{1,1,1,0,0,0x09},{1,1,1,0,0,0x0A},{1,1,1,0,0,0x0B},
  {1,1,1,0,0,0x0C},{1,1,1,0,0,0x0D},{1,1,1,0,0,0x0E},{1,1,1,0,0,0x0F},

  {1,1,1,0,0,0x10},{1,1,1,0,0,0x11},{1,1,1,0,0,0x12},{1,1,1,0,0,0x13},
  {1,1,1,0,0,0x14},{1,1,1,0,0,0x15},{1,1,1,0,0,0x16},{1,1,1,0,0,0x17},
  {1,1,1,0,0,0x18},{1,1,1,0,0,0x19},{1,1,1,0,0,0x1A},{1,1,1,0,0,0x1B},
  {1,1,1,0,0,0x1C},{1,1,1,0,0,0x1D},{1,1,1,0,0,0x1E},{1,1,1,0,0,0x1F},

  {1,1,1,0,0,0x20},{1,1,1,0,0,0x21},{1,1,1,0,0,0x22},{1,1,1,0,0,0x23},
  {1,1,1,0,0,0x24},{1,1,1,0,0,0x25},{1,1,1,0,0,0x26},{1,1,1,0,0,0x27},
  {1,1,1,0,0,0x28},{1,1,1,0,0,0x29},{1,1,1,0,0,0x2A},{1,1,1,0,0,0x2B},
  {1,1,1,0,0,0x2C},{1,1,1,0,0,0x2D},{1,1,1,0,0,0x2E},{1,1,1,0,0,0x2F},

  {1,1,1,0,0,0x30},{1,1,1,0,0,0x31},{1,1,1,0,0,0x32},{1,1,1,0,0,0x33},
  {1,1,1,0,0,0x34},{1,1,1,0,0,0x35},{1,1,1,0,0,0x36},{1,1,1,0,0,0x37},
  {1,1,1,0,0,0x38},{1,1,1,0,0,0x39},{1,1,1,0,0,0x3A},{1,1,1,0,0,0x3B},
  {1,1,1,0,0,0x3C},{1,1,1,0,0,0x3D},{1,1,1,0,0,0x3E},{1,1,1,0,0,0x3F},

  {1,1,1,0,0,0x40},{1,1,1,0,0,0x41},{1,1,1,0,0,0x42},{1,1,1,0,0,0x43},
  {1,1,1,0,0,0x44},{1,1,1,0,0,0x45},{1,1,1,0,0,0x46},{1,1,1,0,0,0x47},
  {1,1,1,0,0,0x48},{1,1,1,0,0,0x49},{1,1,1,0,0,0x4A},{1,1,1,0,0,0x4B},
  {1,1,1,0,0,0x4C},{1,1,1,0,0,0x4D},{1,1,1,0,0,0x4E},{1,1,1,0,0,0x4F},

  {1,1,1,0,0,0x50},{1,1,1,0,0,0x51},{1,1,1,0,0,0x52},{1,1,1,0,0,0x53},
  {1,1,1,0,0,0x54},{1,1,1,0,0,0x55},{1,1,1,0,0,0x56},{1,1,1,0,0,0x57},
  {1,1,1,0,0,0x58},{1,1,1,0,0,0x59},{1,1,1,0,0,0x5A},{1,1,1,0,0,0x5B},
  {1,1,1,0,0,0x5C},{1,1,1,0,0,0x5D},{1,1,1,0,0,0x5E},{1,1,1,0,0,0x5F},

  {1,1,1,0,0,0x60},{1,1,1,0,0,0x61},{1,1,1,0,0,0x62},{1,1,1,0,0,0x63},
  {1,1,1,0,0,0x64},{1,1,1,0,0,0x65},{1,1,1,0,0,0x66},{1,1,1,0,0,0x67},
  {1,1,1,0,0,0x68},{1,1,1,0,0,0x69},{1,1,1,0,0,0x6A},{1,1,1,0,0,0x6B},
  {1,1,1,0,0,0x6C},{1,1,1,0,0,0x6D},{1,1,1,0,0,0x6E},{1,1,1,0,0,0x6F},

  {1,1,1,0,0,0x70},{1,1,1,0,0,0x71},{1,1,1,0,0,0x72},{1,1,1,0,0,0x73},
  {1,1,1,0,0,0x74},{1,1,1,0,0,0x75},{1,1,1,0,0,0x76},{1,1,1,0,0,0x77},
  {1,1,1,0,0,0x78},{1,1,1,0,0,0x79},{1,1,1,0,0,0x7A},{1,1,1,0,0,0x7B},
  {1,1,1,0,0,0x7C},{1,1,1,0,0,0x7D},{1,1,1,0,0,0x7E},{1,1,1,0,0,0x7F},

  {1,1,1,0,0,0x80},{1,1,1,0,0,0x81},{1,1,1,0,0,0x82},{1,1,1,0,0,0x83},
  {1,1,1,0,0,0x84},{1,1,1,0,0,0x85},{1,1,1,0,0,0x86},{1,1,1,0,0,0x87},
  {1,1,1,0,0,0x88},{1,1,1,0,0,0x89},{1,1,1,0,0,0x8A},{1,1,1,0,0,0x8B},
  {1,1,1,0,0,0x8C},{1,1,1,0,0,0x8D},{1,1,1,0,0,0x8E},{1,1,1,0,0,0x8F},

  {1,1,1,0,0,0x90},{1,1,1,0,0,0x91},{1,1,1,0,0,0x92},{1,1,1,0,0,0x93},
  {1,1,1,0,0,0x94},{1,1,1,0,0,0x95},{1,1,1,0,0,0x96},{1,1,1,0,0,0x97},
  {1,1,1,0,0,0x98},{1,1,1,0,0,0x99},{1,1,1,0,0,0x9A},{1,1,1,0,0,0x9B},
  {1,1,1,0,0,0x9C},{1,1,1,0,0,0x9D},{1,1,1,0,0,0x9E},{1,1,1,0,0,0x9F},

  {1,1,1,0,0,0xA0},{1,1,1,0,0,0xA1},{1,1,1,0,0,0xA2},{1,1,1,0,0,0xA3},
  {1,1,1,0,0,0xA4},{1,1,1,0,0,0xA5},{1,1,1,0,0,0xA6},{1,1,1,0,0,0xA7},
  {1,1,1,0,0,0xA8},{1,1,1,0,0,0xA9},{1,1,1,0,0,0xAA},{1,1,1,0,0,0xAB},
  {1,1,1,0,0,0xAC},{1,1,1,0,0,0xAD},{1,1,1,0,0,0xAE},{1,1,1,0,0,0xAF},

  {1,1,1,0,0,0xB0},{1,1,1,0,0,0xB1},{1,1,1,0,0,0xB2},{1,1,1,0,0,0xB3},
  {1,1,1,0,0,0xB4},{1,1,1,0,0,0xB5},{1,1,1,0,0,0xB6},{1,1,1,0,0,0xB7},
  {1,1,1,0,0,0xB8},{1,1,1,0,0,0xB9},{1,1,1,0,0,0xBA},{1,1,1,0,0,0xBB},
  {1,1,1,0,0,0xBC},{1,1,1,0,0,0xBD},{1,1,1,0,0,0xBE},{1,1,1,0,0,0xBF},

  {1,1,1,0,0,0xC0},{1,1,1,0,0,0xC1},{1,1,1,0,0,0xC2},{1,1,1,0,0,0xC3},
  {1,1,1,0,0,0xC4},{1,1,1,0,0,0xC5},{1,1,1,0,0,0xC6},{1,1,1,0,0,0xC7},
  {1,1,1,0,0,0xC8},{1,1,1,0,0,0xC9},{1,1,1,0,0,0xCA},{1,1,1,0,0,0xCB},
  {1,1,1,0,0,0xCC},{1,1,1,0,0,0xCD},{1,1,1,0,0,0xCE},{1,1,1,0,0,0xCF},

  {1,1,1,0,0,0xD0},{1,1,1,0,0,0xD1},{1,1,1,0,0,0xD2},{1,1,1,0,0,0xD3},
  {1,1,1,0,0,0xD4},{1,1,1,0,0,0xD5},{1,1,1,0,0,0xD6},{1,1,1,0,0,0xD7},
  {1,1,1,0,0,0xD8},{1,1,1,0,0,0xD9},{1,1,1,0,0,0xDA},{1,1,1,0,0,0xDB},
  {1,1,1,0,0,0xDC},{1,1,1,0,0,0xDD},{1,1,1,0,0,0xDE},{1,1,1,0,0,0xDF},

  {1,1,1,0,0,0xE0},{1,1,1,0,0,0xE1},{1,1,1,0,0,0xE2},{1,1,1,0,0,0xE3},
  {1,1,1,0,0,0xE4},{1,1,1,0,0,0xE5},{1,1,1,0,0,0xE6},{1,1,1,0,0,0xE7},
  {1,1,1,0,0,0xE8},{1,1,1,0,0,0xE9},{1,1,1,0,0,0xEA},{1,1,1,0,0,0xEB},
  {1,1,1,0,0,0xEC},{1,1,1,0,0,0xED},{1,1,1,0,0,0xEE},{1,1,1,0,0,0xEF},

  {1,1,1,0,0,0xF0},{1,1,1,0,0,0xF1},{1,1,1,0,0,0xF2},{1,1,1,0,0,0xF3},
  {1,1,1,0,0,0xF4},{1,1,1,0,0,0xF5},{1,1,1,0,0,0xF6},{1,1,1,0,0,0xF7},
  {1,1,1,0,0,0xF8},{1,1,1,0,0,0xF9},{1,1,1,0,0,0xFA},{1,1,1,0,0,0xFB},
  {1,1,1,0,0,0xFC},{1,1,1,0,0,0xFD},{1,1,1,0,0,0xFE},{1,1,1,0,0,0xFF},
};

/* Table of font 'bitmaps', really just their character code. */
u8 const serial40x4_font_bitmaps[256] = {
   0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
   0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
   0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,
   0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,
   0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,
   0x28,0x29,0x2A,0x2B,0x2C,0x2D,0x2E,0x2F,
   0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,
   0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F,
   0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,
   0x48,0x49,0x4A,0x4B,0x4C,0x4D,0x4E,0x4F,
   0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,
   0x58,0x59,0x5A,0x5B,0x5C,0x5D,0x5E,0x5F,
   0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,
   0x68,0x69,0x6A,0x6B,0x6C,0x6D,0x6E,0x6F,
   0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,
   0x78,0x79,0x7A,0x7B,0x7C,0x7D,0x7E,0x7F,
   0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,
   0x88,0x89,0x8A,0x8B,0x8C,0x8D,0x8E,0x8F,
   0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,
   0x98,0x99,0x9A,0x9B,0x9C,0x9D,0x9E,0x9F,
   0xA0,0xA1,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,
   0xA8,0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF,
   0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,
   0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF,
   0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,
   0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,
   0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,
   0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF,
   0xE0,0xE1,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,
   0xE8,0xE9,0xEA,0xEB,0xEC,0xED,0xEE,0xEF,
   0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,
   0xF8,0xF9,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF
};

/* Glyph table for the bigfont */
struct fontglyph const serial40x4_bigfont_glyphs[] = {
  { 2, 0,0,0,0, 0x0000 },
  { 4, 3,4,0,0, 0x0000 },
  { 2, 1,4,0,0, 0x000C },
  { 4, 3,4,0,0, 0x0010 },
  { 4, 3,4,0,0, 0x001C },
  { 4, 3,4,0,0, 0x0028 },
  { 4, 3,4,0,0, 0x0034 },
  { 4, 3,4,0,0, 0x0040 },
  { 4, 3,4,0,0, 0x004C },
  { 4, 3,4,0,0, 0x0058 },
  { 4, 3,4,0,0, 0x0064 },
  { 2, 1,4,0,0, 0x0070 },
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

struct font const serial40x4_font = {
  /*    numglyphs = */ 256,
  /*   beginglyph = */ 0,
  /* defaultglyph = */ 32,
  /*            w = */ 1,
  /*            h = */ 1,
  /*       ascent = */ 1,
  /*      descent = */ 0,
  /*       glyphs = */ &serial40x4_font_glyphs,
  /*      bitmaps = */ &serial40x4_font_bitmaps
};
struct font const serial40x4_bigfont = {
  /*    numglyphs = */ 12,
  /*   beginglyph = */ 47,
  /* defaultglyph = */ 47,
  /*            w = */ 4,
  /*            h = */ 4,
  /*       ascent = */ 4,
  /*      descent = */ 0,
  /*       glyphs = */ &serial40x4_bigfont_glyphs,
  /*      bitmaps = */ &serial40x4_bigfont_bitmaps
};

/* Bogus fontstyle nodes */
struct fontstyle_node serial40x4_font_style = {
   /* name = */ "LCD Pseudofont",
   /* size = */ 1,
   /* flags = */ PG_FSTYLE_FIXED,
   /* next = */ NULL,
   /* normal = */ (struct font *) &serial40x4_font,
   /* bold = */ NULL,
   /* italic = */ NULL,
   /* bolditalic = */ NULL,
   /* ulineh = */ 1,
   /* slineh = */ 0,
   /* boldw = */ 0
};
struct fontstyle_node serial40x4_bigfont_style = {
   /* name = */ "Large LCD Font",
   /* size = */ 4,
   /* flags = */ 0,
   /* next = */ NULL,
   /* normal = */ (struct font *) &serial40x4_bigfont,
   /* bold = */ NULL,
   /* italic = */ NULL,
   /* bolditalic = */ NULL,
   /* ulineh = */ 4,
   /* slineh = */ 0,
   /* boldw = */ 0
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

g_error serial40x4_init(void) {
   int i;
   g_error e;
   unsigned long size;
   u8 *p;

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
    
   /* Download custom characters to the LCD */
   write(lcd_fd,"\\1\\g",4);
   write(lcd_fd,cg1,64);
   write(lcd_fd,"\\2\\g",4);
   write(lcd_fd,cg2,64);
   
   return sucess;
}

void serial40x4_close(void) {
   g_free(FB_MEM);   /* And shadow buffer */
   close(lcd_fd);
}

/* Use a shadow buffer to send only changed data to the LCD.
 *
 * Cruftily hardcoded to a split 40x4 Hitachi-compatible LCD
 */
void serial40x4_update(s16 x,s16 y,s16 w,s16 h) {
   int i_lcd,i;
   u8 *oldp,*newp;
   /* Output buffer big enough to hold one frame with worst-case encoding */
   u8 outbuf[1024];
   u8 *outp = outbuf;
   
   /* start comparing the buffers */
   i_lcd = -1;
   oldp = lcd_shadow;
   newp = FB_MEM;
   for (i=0;i<256;oldp++,newp++,i++) {
      if (*oldp == *newp)
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
		      hwrcolor c,struct quad *clip,bool fill, hwrcolor bg,
		      s16 lgop) {

   if (clip && (dest_x<clip->x1 || dest_y<clip->y1 ||
		dest_x>clip->x2 || dest_y>clip->y2))
     return;   

   /* Not at all correct, but until terminal theming support works it's
    * the only way to make pterm look ok */
   if (fill)
     (*vid->pixel)(dest,dest_x,dest_y,0,lgop);
   else {
      struct stdbitmap chbit;
      chbit.bits = chardat;
      chbit.w = chbit.pitch = w;
      chbit.h = h;
      (*vid->blit)(dest,dest_x,dest_y,w,h,(hwrbitmap) &chbit,0,0,PG_LGOP_NONE);
   }
}

void serial40x4_font_newdesc(struct fontdesc *fd, char *name,
			     int size, stylet flags) {
   fd->margin = 0;
   fd->hline = -1;
   fd->italicw = 0;
   if (size>20)     /* Arbitrary number */
     fd->fs = &serial40x4_bigfont_style;
   else
     fd->fs = &serial40x4_font_style;
   fd->font = (struct font *) fd->fs->normal;
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

void serial40x4_message(u32 message, u32 param) {
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
   
   return sucess;
}

/* The End */
