/* $Id: ncurses.c,v 1.27 2002/03/26 03:12:35 instinc Exp $
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
#include <pgserver/appmgr.h>
#include <pgserver/font.h>
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

/******************************************** Fake font */
/* This is a little hack to trick PicoGUI's text rendering */

/* We only keep 1 byte here, filling it in later with the character code */
u8 ncurses_font_bitmap;

struct font const ncurses_font = {
   w: 1, 
   h: 1,
   defaultglyph: ' ',
   ascent: 1,
   descent: 0,
   bitmaps: &ncurses_font_bitmap,
   glyphs: NULL,
};

/* Bogus fontstyle node */
struct fontstyle_node ncurses_font_style = {
   name: "Ncurses Pseudofont",
   size: 1,
   flags: PG_FSTYLE_FIXED,
   next: NULL,
   normal: (struct font *) &ncurses_font,
   bold: NULL,
   italic: NULL,
   bolditalic: NULL,
   boldw: 0
};
        
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
   for (p=ncurses_screen,size=vid->xres*vid->yres;size;size--,p++)
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

void ncurses_update(s16 x,s16 y,s16 w,s16 h) {
   refresh();

#ifdef DRIVER_GPM
   /* Show the cursor */
   if(gpm_last_event.type)
      GPM_DRAWPOINTER(&gpm_last_event);
#endif
}

/**** Hack the normal font rendering a bit so we use regular text */

void ncurses_font_newdesc(struct fontdesc *fd, const u8 *name,
			  int size, int flags) {
   fd->font = (struct font *) &ncurses_font;
   fd->margin = 0;
   fd->hline = -1;
   fd->italicw = 0;
   fd->fs = &ncurses_font_style;
}

struct fontglyph const *ncurses_font_getglyph(struct fontdesc *fd, int ch) {
  /* Fake glyph and bitmap to return */
  static struct fontglyph fakeglyph;

  fakeglyph.encoding  = ch;
  fakeglyph.bitmap    = 0;
  ncurses_font_bitmap = ch;
  fakeglyph.dwidth    = 1;
  fakeglyph.w         = 1;
  fakeglyph.h         = 1;
  fakeglyph.x         = 0;
  fakeglyph.y         = 0;
  
  return &fakeglyph;
}

void ncurses_charblit(hwrbitmap dest, u8 *chardat,s16 dest_x,
		      s16 dest_y,s16 w,s16 h,s16 lines,s16 angle,
		      hwrcolor c,struct quad *clip,s16 lgop) {
   chtype *location;
   
   if (lgop != PG_LGOP_NONE) {
      def_charblit(dest,chardat,dest_x,dest_y,w,h,lines,angle,c,clip,lgop);
      return;
   }
   
   /* Make sure we're within clip */
   if (clip && (dest_x<clip->x1 || dest_y<clip->y1 ||
		dest_x>clip->x2 || dest_y>clip->y2))
     return;
   
   /* Get the previous contents, strip out all but the background,
    * and add our new foreground and text */
   location = ncurses_screen + dest_x + vid->xres * dest_y;
   *location = COLOR_PAIR((PAIR_NUMBER(*location & (~A_CHARTEXT)) & 0x38) | 
			  ((PAIR_NUMBER(c) & 0x38)>>3)) | (c & A_BOLD) | (*chardat);
     
   /* Send it */
   mvaddch(dest_y,dest_x,*location);
}

/**** We use a ncurses character cell as our hwrcolor */

/* This can handle input colors in different formats, always returning
 * a ncurses attribute value */

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

/**** A hack to turn off the picogui sprite cursor */

extern void def_sprite_show(struct sprite *spr);
void ncurses_sprite_show(struct sprite *spr) {
   if (spr==cursor) {
      spr->visible = 0;
    
#ifdef DRIVER_GPM
      /* Do it ourselves */
      if(gpm_last_event.type)
         GPM_DRAWPOINTER(&gpm_last_event);
#endif
   }
      
   def_sprite_show(spr);
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
   
   v->font_newdesc = &ncurses_font_newdesc;
   v->font_getglyph = &ncurses_font_getglyph;
   v->charblit = &ncurses_charblit;

   v->sprite_show = &ncurses_sprite_show;
   
   return success;
}

/* The End */
