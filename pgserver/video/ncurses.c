/* $Id: ncurses.c,v 1.32 2002/11/04 02:45:46 micahjd Exp $
 *
 * ncurses.c - ncurses driver for PicoGUI. This lets PicoGUI make
 *             nice looking and functional text-mode GUIs.
 *             Note that this driver ignores fonts and bitmaps.
 *             Because units are now in characters, not pixels,
 *             you should probably load a theme designed for this.
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
   if (dest || (lgop!=PG_LGOP_NONE))
     def_pixel(dest,x,y,c,lgop);
   else
     mvaddch(y,x,ncurses_screen[x + vid->xres * y] = c);
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

   if (c & PGCF_TEXT_ASCII) {
      /* Normal character:  0x20BBFFCC
       * BB = background
       * FF = foreground
       * CC = character
       */
      
      return COLOR_PAIR( ((c & 0x070000)>>13) | ((c & 0x000700)>>8) ) |
	((c & 0x000800) ? A_BOLD : 0) | (c & 0x0000FF);
   }
   
   else if (c & PGCF_TEXT_ACS) {
      /* ACS character:  0x40BBFFCC
       * BB = background
       * FF = foreground
       * CC = ACS character code
       */
      
      return COLOR_PAIR( ((c & 0x070000)>>13) | ((c & 0x000700)>>8) ) |
	((c & 0x000800) ? A_BOLD : 0) | acs_map[c & 0x0000FF];
   }

   else if (c & PGCF_ALPHA) {
     /* Default conversion for alpha values (premultiply) */
     return def_color_pgtohwr(c);
   }

   else {
     /* RGB value interpreted as a background attribute */
      
     int sc = 7;
     if ((c & 0xFF0000) > 0x400000) sc |= 32;
     if ((c & 0x00FF00) > 0x004000) sc |= 16;
     if ((c & 0x0000FF) > 0x000040) sc |= 8;
     return (COLOR_PAIR(sc) | ( ((c&0xFF0000) > 0xA00000) || 
				((c&0x00FF00) > 0x00A000) || 
				((c&0x0000FF) > 0x0000A0) ? A_BOLD : 0)) | ' ';
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
   
   return success;
}

/* The End */
