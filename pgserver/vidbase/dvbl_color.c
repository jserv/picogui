/* $Id: dvbl_color.c,v 1.1 2002/04/03 08:08:41 micahjd Exp $
 *
 * dvbl_color.c - This file is part of the Default Video Base Library,
 *                providing the basic video functionality in picogui but
 *                easily overridden by drivers.
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
 */

#include <pgserver/common.h>

#ifdef DEBUG_VIDEO
#define DEBUG_FILE
#endif
#include <pgserver/debug.h>

#include <pgserver/video.h>
#include <pgserver/font.h>
#include <pgserver/render.h>

/******* colors */

hwrcolor def_color_pgtohwr(pgcolor c) {
  if (c & PGCF_ALPHA) {
    /* ARGB conversion, just premultiply the RGB color */
    return mkcolora( getalpha(c),
		     (getred(c)   * getalpha(c)) >> 7,
		     (getgreen(c) * getalpha(c)) >> 7,
		     (getblue(c)  * getalpha(c)) >> 7
		     );
  }
  else if (vid->bpp==1) {
    /* Black and white, tweaked for better contrast */
    return (getred(c)+getgreen(c)+getblue(c)) >= 382;
  }
  else if(vid->bpp<8) {
    /* grayscale */
    return (getred(c)+getgreen(c)+getblue(c)) * ((1<<vid->bpp)-1) / 765;
  }
  else if (vid->bpp==8) {
    /* 2-3-3 color */
    return ((getred(c) & 0xC0) |
	    ((getgreen(c) >> 2) & 0x38) |
	    ((getblue(c) >> 5)));
  }
  else if (vid->bpp==16) {
    /* 5-6-5 color */
    return (((getred(c) << 8) & 0xF800) |
	    ((getgreen(c) << 3) & 0x07E0) |
	    ((getblue(c) >> 3) & 0x001F));
  }
  else if (vid->bpp==15) {
    /* 5-5-5 color */
    return (((getred(c) << 7) & 0x7C00) |
	    ((getgreen(c) << 2) & 0x03E0) |
	    ((getblue(c) >> 3) & 0x001F));
  }
  else if (vid->bpp==12) {
    /* 4-4-4 color */
    return (((getred(c) << 4) & 0x0F00) |
	    (getgreen(c) & 0x00F0) |
	    ((getblue(c) >> 4) & 0x000F));
  }
  else
    /* True color */
    return c;
}

pgcolor def_color_hwrtopg(hwrcolor c) {
  if (c & PGCF_ALPHA) {
    /* ARGB conversion, un-premultiply the RGB color */
    return mkcolora( getalpha(c),
		     (getred(c)   << 7) / getalpha(c),
		     (getgreen(c) << 7) / getalpha(c),
		     (getblue(c)  << 7) / getalpha(c)
		     );
  }
  else if (vid->bpp==1) {
    return c ? 0xFFFFFF : 0x000000;  
  }
  else if (vid->bpp<8) {
    /* grayscale */
    unsigned char gray = c * 255 / ((1<<vid->bpp)-1);
    return mkcolor(gray,gray,gray);
  }
  else if (vid->bpp==8) {
    /* 2-3-3 color */  
    u8 r = c&0xC0, g = (c&0x38)<<2, b = c<<5;
    return mkcolor(r | (r>>2) | (r>>4) | (r>>6),
		   g | (g>>3) | (g>>6),
		   b | (b>>3) | (b>>6));
  }
  else if (vid->bpp==16) {
     /* 5-6-5 color */
     u8 r = (c&0xF800)>>8, g = (c&0x07E0)>>3, b = (c&0x001F)<<3;
     return mkcolor( r | (r>>5),
		     g | (g>>6),
		     b | (b>>5));
  }
  else if (vid->bpp==15) {
     /* 5-5-5 color */
     return mkcolor( (c&0x7C00) >> 7,
		     (c&0x03E0) >> 2,
		     (c&0x001F) << 3 );
  }
  else if (vid->bpp==12) {
     /* 4-4-4 color */
     return mkcolor( (c&0x0F00) >> 4,
		     (c&0x00F0),
		     (c&0x000F) << 4 );
  }
  else
    /* True color */
    return c;
}


/* The End */
