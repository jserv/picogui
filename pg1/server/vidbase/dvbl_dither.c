/* $Id$
 *
 * dvbl_dither.c - This file is part of the Default Video Base Library,
 *                 providing the basic video functionality in picogui but
 *                 easily overridden by drivers.
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2003 Micah Dowty <micahjd@users.sourceforge.net>
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


/*   Start writing dithered data into a bitmap. Returns a dithering structure
 *   that's used to keep track of the position in the image. It's assumed that
 *   image data will be fed in top-down unless vflip is 1, in which case it's
 *   assumed bottom-up.
 */
g_error def_dither_start(hwrdither *d, hwrbitmap dest, 
			 int vflip, int x, int y, int w, int h) {
  g_error e;
  struct stddither *sd;
  int buffersize;
  int i;

  /* Allocate a stddither */
  e = g_malloc((void**)d, sizeof(struct stddither));
  errorcheck;
  sd = (struct stddither *) *d;
  memset(sd,0,sizeof(struct stddither));

  sd->x    = x;
  sd->y    = y;
  sd->w    = w;
  sd->h    = h;
  sd->dest = dest;

  /* If we're flipped vertically, start at the bottom */
  if (vflip) {
    sd->y += h-1;
    sd->dy = -1;
  }
  else
    sd->dy = 1;

  /* Allocate two line buffers, and point to the beginning of both.
   * We allocate the buffers two pixels larger than we need to avoid
   * having to check for edge cases.
   */
  
  buffersize = sizeof(int) * (sd->w + 2);

  /* We need buffers for each channel... */
  for (i=0;i<3;i++) {
    
    e = g_malloc((void**)&sd->current_line[i], buffersize);
    errorcheck;
    memset(sd->current_line[i],0,buffersize);
    
    e = g_malloc((void**)&sd->next_line[i], buffersize);
    errorcheck;
    memset(sd->next_line[i],0,buffersize);
    
    sd->current_line[i]++;
    sd->next_line[i]++;
    sd->current[i] = sd->current_line[i];
    sd->below[i] = sd->next_line[i];
  }  

  return success;
}


/*
 *   Dither this pixel and store it in the bitmap
 */
void def_dither_store(hwrdither d, pgcolor pixel, s16 lgop) {
  pgcolor pgc, quantized;
  hwrcolor hwc;
  int error[3];
  int c;            /* Channel */
  int r,g,b;        /* Temp values for clamping */
  struct stddither *sd = (struct stddither *) d;

  /* If this pixel has extra flags, like alpha channel, pass it straight through */
  if (pixel>>24) {
    vid->pixel(sd->dest, sd->x + sd->i, sd->y, vid->color_pgtohwr(pixel), lgop);    
  }
  else {

    /* Put this pixel into the buffer */
    sd->current[0][0] += getred(pixel) << 4;
    sd->current[1][0] += getgreen(pixel) << 4;
    sd->current[2][0] += getblue(pixel) << 4;
    
    /* Clamp the buffer contents at this pixel */
    r = sd->current[0][0] >> 4;
    if (r<0)   r = 0;
    if (r>255) r = 255;
    g = sd->current[1][0] >> 4;
    if (g<0)   g = 0;
    if (g>255) g = 255;
    b = sd->current[2][0] >> 4;
    if (b<0)   b = 0;
    if (b>255) b = 255;
    
    /* Convert it to a pgcolor/hwcolor and plot it */
    pgc = mkcolor(r,g,b);
    hwc = vid->color_pgtohwr(pgc);
    vid->pixel(sd->dest, sd->x + sd->i, sd->y, hwc, lgop);
    
    /* Compute the quantization error */
    quantized = vid->color_hwrtopg(hwc);
    error[0] = ((int)  getred(pgc)) - ((int)  getred(quantized));
    error[1] = ((int)getgreen(pgc)) - ((int)getgreen(quantized));
    error[2] = ((int) getblue(pgc)) - ((int) getblue(quantized));
    
    for (c=0;c<3;c++) {
      /* Distribute the error using the floyd-steinberg filter:
       *     X 7
       *   3 5 1
       */
      sd->current[c][1] += 7 * error[c];
      sd->below[c][-1]  += 3 * error[c];
      sd->below[c][0]   += 5 * error[c];
      sd->below[c][1]   +=     error[c];
    }
  }
  
  /* Next pixel.. */
  sd->i++;
  for (c=0;c<3;c++) {
    sd->current[c]++;
    sd->below[c]++;
  }
  
  /* Next line... */
  if (sd->i == sd->w) {
    sd->i = 0;
    for (c=0;c<3;c++) {
      sd->current[c] = sd->current_line[c];
      sd->below[c]   = sd->next_line[c];
      memcpy(sd->current_line[c], sd->next_line[c], sizeof(int)*sd->w);
      memset(sd->next_line[c], 0, sizeof(int)*sd->w);
    }
    sd->h -= sd->dy;
    sd->y += sd->dy;
  }
}

/*
 *   Free the dithering structure
 */
void def_dither_finish(hwrdither d) {
  struct stddither *sd = (struct stddither *) d;
  int i;
  for (i=0;i<3;i++) {
    g_free(sd->current_line[i]-1);
    g_free(sd->next_line[i]-1);
  }
  g_free(sd);
}

/* The End */
