/* $Id: video.c,v 1.28 2001/02/28 00:19:07 micahjd Exp $
 *
 * video.c - handles loading/switching video drivers, provides
 *           default implementations for video functions
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
#include <pgserver/divtree.h>

/******************************************** Utils */

/* Vidlib vars */
struct vidlib *vid;
struct vidlib vidlib_static;
struct sprite *spritelist;
int upd_x;
int upd_y;
int upd_w;
int upd_h;
hwrcolor textcolors[16];   /* Table for converting 16 text colors
			      to hardware colors */

/* Trig table used in hwr_gradient (sin*256 for theta from 0 to 90) */
unsigned char trigtab[] = {
  0x00,0x04,0x08,0x0D,0x11,0x16,0x1A,0x1F,0x23,0x28,
  0x2C,0x30,0x35,0x39,0x3D,0x42,0x46,0x4A,0x4F,0x53,
  0x57,0x5B,0x5F,0x64,0x68,0x6C,0x70,0x74,0x78,0x7C,
  0x80,0x83,0x87,0x8B,0x8F,0x92,0x96,0x9A,0x9D,0xA1,
  0xA4,0xA7,0xAB,0xAE,0xB1,0xB5,0xB8,0xBB,0xBE,0xC1,
  0xC4,0xC6,0xC9,0xCC,0xCF,0xD1,0xD4,0xD6,0xD9,0xDB,
  0xDD,0xDF,0xE2,0xE4,0xE6,0xE8,0xE9,0xEB,0xED,0xEE,
  0xF0,0xF2,0xF3,0xF4,0xF6,0xF7,0xF8,0xF9,0xFA,0xFB,
  0xFC,0xFC,0xFD,0xFE,0xFE,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF
};

/* Little function to skip to the next value in an ASCII file */
void ascskip(unsigned char **dat,unsigned long *datlen) {
  while (*datlen) {
    if (**dat == ' ' || **dat == '\t' || **dat == '\n' || **dat == '\r') {
      (*dat)++;
      (*datlen)--;
    }
    else if (**dat == '#') {
      while ((*datlen) && (**dat != '\n') && (**dat != '\r')) {
	(*dat)++;
	(*datlen)--;
      }
    }
    else
      return;
  }
}

/* Read a number from ascii data */
int ascread(unsigned char **dat,unsigned long *datlen) {
  char buf[10];
  char *p = buf;
  int buflen=9;
  ascskip(dat,datlen);
  while ((**dat != ' ') && (**dat != '\t') && (**dat != '\n') &&
	 (**dat != '\r') && *datlen && buflen) {
    *(p++) = *((*dat)++);
    (*datlen)--;
    buflen--;
  }
  *p = 0;
  return atoi(buf);
}

/* Sprite helper functions */
g_error new_sprite(struct sprite **ps,int w,int h) {
  g_error e;
  
  e = g_malloc((void**)ps,sizeof(struct sprite));
  errorcheck;
  memset(*ps,0,sizeof(struct sprite));
  (*ps)->ox = -1;
  (*ps)->w = w;
  (*ps)->h = h;
  (*vid->bitmap_new)(&(*ps)->backbuffer,w,h);
  (*ps)->next = spritelist;
  (*ps)->visible = 1;
   
  spritelist = *ps;
   
  (*vid->sprite_show)(*ps);

  return sucess;
}

void free_sprite(struct sprite *s) {
  struct sprite *n;

  (*vid->sprite_hide)(s);
   
  /* Remove from the sprite list */
  if (s==spritelist)
    spritelist = s->next;
  else {
    n = spritelist;
    while (n->next) {
      if (n->next == s) {
	n->next = s->next;
	break;
      }
      n = n->next;
    }
  }

  (*vid->bitmap_free)(s->bitmap);
  (*vid->bitmap_free)(s->mask);
  (*vid->bitmap_free)(s->backbuffer);
  g_free(s);
}

/******************************************** Vidlib admin functions */

/* Let the driver register itself, and initialize things */
g_error load_vidlib(g_error (*regfunc)(struct vidlib *v),
		  int xres,int yres,int bpp,unsigned long flags) {
  g_error e;
  unsigned char i;

  /* Unload */
  if (vid) 
    (*vid->close)();

  /* Clear it */
  vid = &vidlib_static;
  memset(vid,0,sizeof(struct vidlib));
  vid->close = &emulate_dos;
  vid->update = &def_update;
  
  /* Device specifics */
  e = (*regfunc)(vid);
  if (iserror(e)) {
    vid = NULL;
    return e;
  }

  inlib_main = NULL;

  /* Load new driver */
  e = (*vid->init)(xres,yres,bpp,flags);
  if (iserror(e)) {
    vid = NULL;
    return e;
  }

  /* Generate text colors table */
  for (i=0;i<16;i++)
    textcolors[i] = (*vid->color_pgtohwr)
      ( (i & 0x08) ?
	(((i & 0x04) ? 0xFF0000 : 0) |
	 ((i & 0x02) ? 0x00FF00 : 0) |
	 ((i & 0x01) ? 0x0000FF : 0)) :
	(((i & 0x04) ? 0x800000 : 0) |
	 ((i & 0x02) ? 0x008000 : 0) |
	 ((i & 0x01) ? 0x000080 : 0)) );
	
  return sucess;
}

g_error (*find_videodriver(const char *name))(struct vidlib *v) {
  struct vidinfo *p = videodrivers;
  while (p->name) {
    if (!strcmp(name,p->name))
      return p->regfunc;
    p++;
  }
  return NULL;
}

void add_updarea(int x,int y,int w,int h) {
  if (upd_w) {
    if (x < upd_x) {
      upd_w += upd_x - x;
      upd_x = x;
    }
    if (y < upd_y) {
      upd_h += upd_y - y;
      upd_y = y;
    }
    if ((w+x) > (upd_x+upd_w))
      upd_w = w+x-upd_x;
    if ((h+y) > (upd_y+upd_h))
      upd_h = h+y-upd_y;
  }
  else {
    upd_x = x;
    upd_y = y;
    upd_w = w;
    upd_h = h;
  }
}

/* Update and reset the update rectangle */
void realize_updareas(void) {
  /* This lock is an effort to fix a bug observed while running in SDL:
   * while blitting the update rectangles, another event is recieved,
   * causing this to be entered twice. Apparently X doesn't like that :)
   */
  static unsigned char lock = 0;

  if (lock) return;
  lock = 1;

   if (upd_w) {
      if (upd_x<0) {
	 upd_w += upd_x;
	 upd_x = 0;
      }
      if (upd_y<0) {
	 upd_h += upd_y;
	 upd_y = 0;
      }
      if ((upd_x+upd_w)>vid->xres)
	upd_w = vid->xres-upd_x;
      if ((upd_y+upd_h)>vid->yres)
	upd_h = vid->yres-upd_y;
#ifdef DEBUG_VIDEO
      /* Show update rectangles */
      //      (*vid->frame)(upd_x,upd_y,upd_w,upd_h,(*vid->color_pgtohwr)(0xFF0000));
#endif
      (*vid->update)(upd_x,upd_y,upd_w,upd_h);
      upd_x = upd_y = upd_w = upd_h = 0;
   } 
   
   lock = 0;
}

/* The End */

