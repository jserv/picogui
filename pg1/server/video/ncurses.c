/* $Id$
 *
 * ncurses.c - ncurses driver for PicoGUI. This lets PicoGUI make
 *             nice looking and functional text-mode GUIs.
 *             Note that this driver ignores fonts and bitmaps.
 *             Because units are now in characters, not pixels,
 *             you should probably load a theme designed for this.
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
 * Lalo Martins <lalo@laranja.org>
 * 
 */

#include <curses.h>

/* curses.h already defines bool, so tell disable the 
 * bool definition in pgserver/common.h
 */
#define NO_BOOL

#include <pgserver/common.h>

#include <pgserver/video.h>
#include <pgserver/input.h>
#include <pgserver/render.h>

#ifdef DRIVER_GPM
#include <gpm.h>
#endif

/* Buffer with the current status of the screen */
chtype *ncurses_screen;

#ifdef DRIVER_GPM
/* The most recent mouse event, exported by the gpm input driver */
extern Gpm_Event gpm_last_event;
#endif


/******************************************** Implementations */

g_error ncurses_init(void) {
   chtype rgbmap[] = { COLOR_BLACK, COLOR_BLUE, COLOR_GREEN, COLOR_CYAN, 
	COLOR_RED, COLOR_MAGENTA, COLOR_YELLOW, COLOR_WHITE };
   int f,b;
   g_error e;
   u32 size;
   u32 *p;

   /* Load the ncursesinput driver, and let it initialize
    * ncurses for us. */
   e = load_inlib(&ncursesinput_regfunc,&inlib_main);
   errorcheck;
   
   /* Set colors */
   for (b=0;b<8;b++)
     for (f=0;f<8;f++)
       init_pair((b<<3) + f,rgbmap[f],rgbmap[b]);
   
   /* Save the actual video mode */
   vid->xres = COLS;
   vid->yres = LINES;
   vid->bpp  = sizeof(chtype)*8;    /* Our pixel is a curses chtype */
   vid->display = NULL;
   
   /* Allocate our buffer */
   e = g_malloc((void**) &ncurses_screen,vid->xres * vid->yres * sizeof(chtype));
   errorcheck;
   for (p=(u32 *)ncurses_screen,size=vid->xres*vid->yres;size;size--,p++)
     *p = ' ';
   
   return success;
}

void ncurses_close(void) {
   /* Take out our input driver */
   unload_inlib(inlib_main);

   g_free(ncurses_screen);
}

void ncurses_pixel(hwrbitmap dest,s16 x,s16 y,hwrcolor c,s16 lgop) {
  if (dest || (lgop!=PG_LGOP_NONE)) {
    def_pixel(dest,x,y,c,lgop);
    return;
  }
  ncurses_screen[x + vid->xres * y] = c;

  if (c & PGCF_TEXT_ASCII) {
    /* Normal character:  0x20BBFFCC
     * BB = background
     * FF = foreground
     * CC = character
     */

    c = COLOR_PAIR( ((c & 0x070000)>>13) | ((c & 0x000700)>>8) ) |
      ((c & 0x000800) ? A_BOLD : 0) | (c & 0x0000FF);
  }
  else if (c & PGCF_TEXT_ACS) {
    /* ACS character:  0x40BBFFCC
     * BB = background
     * FF = foreground
     * CC = ACS character code
     */
    
    c = COLOR_PAIR( ((c & 0x070000)>>13) | ((c & 0x000700)>>8) ) |
      ((c & 0x000800) ? A_BOLD : 0) | acs_map[c & 0x0000FF];
  }
  else {
    /* RGB value interpreted as a background attribute */
    
    int sc = 7;
    if ((c & 0xFF0000) > 0x400000) sc |= 32;
    if ((c & 0x00FF00) > 0x004000) sc |= 16;
    if ((c & 0x0000FF) > 0x000040) sc |= 8;
    c = (COLOR_PAIR(sc) | ( ((c&0xFF0000) > 0xA00000) || 
			    ((c&0x00FF00) > 0x00A000) || 
			    ((c&0x0000FF) > 0x0000A0) ? A_BOLD : 0)) | ' ';
  }
  
  mvaddch(y,x,c);
}

hwrcolor ncurses_getpixel(hwrbitmap src,s16 x,s16 y) {
   if (src)
     return def_getpixel(src,x,y);
   else
     return ncurses_screen[x + vid->xres * y];
}

void ncurses_update(hwrbitmap d,s16 x,s16 y,s16 w,s16 h) {
   refresh();

#ifdef DRIVER_GPM
   /* Show the cursor */
   if(gpm_last_event.type)
      GPM_DRAWPOINTER(&gpm_last_event);
#endif
}

hwrcolor ncurses_color_pgtohwr(pgcolor c) {
  return c;
}

hwrcolor ncurses_color_hwrtopg(pgcolor c) {
  /* Convert the background to an RGB color, quantized to the 8 colors we get */

  if (c & (PGCF_TEXT_ASCII | PGCF_TEXT_ACS))
    return ((c & 0x00040000) ? 0xFF0000 : 0) |
           ((c & 0x00020000) ? 0x00FF00 : 0) |
           ((c & 0x00010000) ? 0x0000FF : 0);

  return c & 0xFFFFFF;
}

/******************************************** Image special-casing (ascii art) */

g_error ncurses_bitmap_load(hwrbitmap *bmp, const u8 *data, u32 datalen) {
  g_error e;

  if (datalen > 2)
    if (data[0] == 'A' && data[1] == 'A')
      {
	/* FIXME: should probably support multi-line art */
	fprintf(stderr, "ncurses: reading ascii art of len %d\n", datalen-2);
	e = vid->bitmap_new(bmp, datalen-2, 1, 8);
	errorcheck;
	(*bmp)->bpp = 0;
	if (data[2] < 32)
	  /* color-coded */
	  (*bmp)->w = (datalen-3) / 2;
	memcpy ((*bmp)->bits, data + 2, datalen-2);
	return success;
      }
  fprintf(stderr, "ncurses: reading bitmap, not ascii art, of len %d (%c%c)\n", datalen,
	  data[0], data[1]);
  return def_bitmap_load(bmp, data, datalen);
}

void ncurses_blit(hwrbitmap dest, s16 x, s16 y, s16 w, s16 h, hwrbitmap src,
	      s16 src_x, s16 src_y, s16 lgop) {
  int i;
  hwrcolor c;
  u8 *p;

  if (!src) {
    fprintf(stderr, "ncurses: blitting non-ascii art\n");
    def_blit(dest, x, y, w, h, src, src_x, src_y, lgop);
    return;
  }

  if (src->bpp) {
    fprintf(stderr, "ncurses: blitting non-ascii art\n");
    def_blit(dest, x, y, w, h, src, src_x, src_y, lgop);
    return;
  }

  fprintf(stderr, "ncurses: blitting ascii art\n");
  if (src->bits[0] >= 32)
    {
      /* just characters (actually slower as we have to get current attributes at the pixels) */
      for (i=0; i < src->w; i++)
	{
	  c = ncurses_getpixel (dest, x+i, y);
	  if (c & (PGCF_TEXT_ASCII | PGCF_TEXT_ACS)) {
	    /* yay! simplest case */
	    if (c & PGCF_TEXT_ACS) {
	      c &= ~PGCF_TEXT_ACS;
	      c |= PGCF_TEXT_ASCII;
	    }
	    c &= ~0xff;  /* forget the old character */
	  }
	  else {
	    /* FIXME - I don't know how to convert an RGB color into the color code ncurses_pixel wants :-( */
	    c = 0x40000F00;   /* white on black */
	  }
	  c |= src->bits[i];
	  ncurses_pixel (dest, x+i, y, c, lgop);
	}
    }
  else if (src->bits[0] == 1)
    {
      /* color-coded pairs, but we're supposed to use the bg color of the "old" pixel */
      for (i=0, p=src->bits+1; i < src->w; i++, p+=2)
	{
	  c = ncurses_getpixel (dest, x+i, y);
	  if (c & (PGCF_TEXT_ASCII | PGCF_TEXT_ACS)) {
	    /* yay! simplest case */
	    if (c & PGCF_TEXT_ACS) {
	      c &= ~PGCF_TEXT_ACS;
	      c |= PGCF_TEXT_ASCII;
	    }
	    c &= ~0xffff;  /* forget the old character and foreground */
	  }
	  else {
	    /* FIXME - I don't know how to convert an RGB color into the color code ncurses_pixel wants :-( */
	    c = 0x40000000;   /* black background */
	  }
	  c |= p[1] | ((p[0] & 0xff) <<  8);
	  ncurses_pixel (dest, x+i, y, c, lgop);
	}
    }
  else
    {
      /* old-style color-coded pairs */
      for (i=0, p=src->bits+1; i < src->w; i++, p+=2)
	{
	  c = PGCF_TEXT_ASCII |
	    ((p[0] & 0x0f) <<  8) | ((p[0] & 0x0f) << 12) |
	    ((p[0] & 0xf0) << 12) | ((p[0] & 0xf0) << 16) |
	    p[1];
	  ncurses_pixel (dest, x+i, y, c, lgop);
	}
    }
}
/******************************************** Driver registration */

g_error ncurses_regfunc(struct vidlib *v) {
   setvbl_default(v);
   
   v->init = &ncurses_init;
   v->close = &ncurses_close;
   v->pixel = &ncurses_pixel;
   v->getpixel = &ncurses_getpixel;
   v->update = &ncurses_update;  
   v->color_pgtohwr = &ncurses_color_pgtohwr;
   v->color_hwrtopg = &ncurses_color_hwrtopg;
   v->bitmap_load = &ncurses_bitmap_load;
   v->blit = &ncurses_blit;
   
   return success;
}

/* The End */
