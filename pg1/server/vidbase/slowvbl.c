/* $Id$
 *
 * Video Base Library:
 * slowvbl.c - intentionally slow VBL for debugging
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
#include <pgserver/configfile.h>
#include <pgserver/video.h>

#include <stdlib.h>	/* strtol */

/* Save the original pixel function so we can call it */
void (*slowvbl_original_pixel)(hwrbitmap dest, s16 x,s16 y,hwrcolor c,s16 lgop);

/* Config parameters */
int slowvbl_delay;
int slowvbl_updatefreq;
int slowvbl_updatetype;
int slowvbl_hilight;
int slowvbl_fastsprites;
hwrcolor slowvbl_hilightcolor;

/* Note whether we're doing a sprite update now */
int slowvbl_in_sprite_update;

void slowvbl_pixel(hwrbitmap dest, s16 x,s16 y,hwrcolor c,s16 lgop) {
  static int count = 0;
  int i;

  if ((slowvbl_fastsprites && slowvbl_in_sprite_update)) {
    (*slowvbl_original_pixel)(dest,x,y,c,lgop);
    return;
  }

  if (slowvbl_hilight) {
    hwrcolor old = vid->getpixel(dest,x,y);
    (*slowvbl_original_pixel)(dest,x,y,slowvbl_hilightcolor,PG_LGOP_NONE);
    vid->update(dest,x,y,1,1);
    (*slowvbl_original_pixel)(dest,x,y,old,PG_LGOP_NONE);
  }

  (*slowvbl_original_pixel)(dest,x,y,c,lgop);

  /* Need update? */
  if (++count >= slowvbl_updatefreq) {
    switch (slowvbl_updatetype) {

    case 1: /* line */
      vid->update(dest,0,y,vid->xres,1);
      break;

    case 2: /* frame */
      vid->update(dest,0,0,vid->xres,vid->yres);
      break;

    default: /* pixel */
      vid->update(dest,x,y,1,1);
      break;

    }
    count = 0;
  }

  /* Yeah, this is disgusting...
   * but usleep() was still too much delay
   */
  for (i=slowvbl_delay;i;i--);
}

extern void def_sprite_update(struct sprite *s);
void slowvbl_sprite_update(struct sprite *s) {
  slowvbl_in_sprite_update++;
  def_sprite_update(s);
  slowvbl_in_sprite_update--;
}

/* Load our driver functions into a vidlib.
 * We assume that the correct VBL for this color depth has already
 * been loaded, and we're now being loaded on top of that.
 */
void setvbl_slowvbl(struct vidlib *vid) {
  hwrcolor (*slowvbl_original_getpixel)(hwrbitmap src, s16 x,s16 y);

  /* Save old functions */
  slowvbl_original_pixel = vid->pixel;
  slowvbl_original_getpixel = vid->getpixel;

  /* Reset all the formerly accelerated primitives */
  setvbl_default(vid);

  /* Set up our pixel wrapper */
  vid->pixel          = &slowvbl_pixel;
  vid->getpixel       = slowvbl_original_getpixel;
  vid->sprite_update  = &slowvbl_sprite_update;

  /* get config params */
  slowvbl_delay = get_param_int("slowvbl","delay",0);
  slowvbl_updatefreq = get_param_int("slowvbl","updatefreq",3);
  slowvbl_updatetype = get_param_int("slowvbl","updatetype",0);
  slowvbl_hilight = get_param_int("slowvbl","hilight",1);
  slowvbl_fastsprites = get_param_int("slowvbl","fastsprites",1);
  slowvbl_hilightcolor = strtol(get_param_str("slowvbl","hilightcolor","FFFF00"),NULL,16);
}

/* The End */

