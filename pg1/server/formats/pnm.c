/* $Id$
 *
 * pnm.c - Functions to convert any of the pbmplus formats (PGM, PBM, PPM)
 *         collectively referred to as PNM
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

#include <stdlib.h>
#include <pgserver/common.h>
#include <pgserver/video.h>

/**************************** Detect */

/* Use the P* header as a 'magic number' */
bool pnm_detect(const u8 *data, u32 datalen) {
   return (datalen > 5) && (data[0] == 'P') && 
          (data[1] > '0') && (data[1] < '7');
}

/**************************** Load */

/* Little function to skip to the next value in an ASCII file */
void ascskip(const u8 **dat,u32 *datlen) {
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
int ascread(const u8 **dat,u32 *datlen) {
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

g_error pnm_load(hwrbitmap *hbmp, const u8 *data, u32 datalen) {
  /* Convert from any of the pbmplus formats in binary or ascii */
  char format;
  int bin = 0;
  int bpp;
  int has_maxval=1;
  int x,y,w,h,max;
  int val,bit,r,g,b;
  g_error e;
  g_error efmt = mkerror(PG_ERRT_BADPARAM,48);
#ifdef CONFIG_DITHER
  hwrdither dither;
#endif

  ascskip(&data,&datalen);
  if (!datalen) return efmt;
  if (*(data++) != 'P') return efmt; datalen--;
  format = *(data++); datalen--;
  /* This decides whether the format is ascii or binary, and if it's
     PBM, PGM, or PPM. */
  switch (format) {
  case '4':
    bin=1;
  case '1':
    bpp=1;
    has_maxval=0;
    break;
  case '5':
    bin=1;
  case '2':
    bpp=8;
    break;
  case '6':
    bin=1;
  case '3':
    bpp=24;
    break;
  default:
    return efmt;
  }
  w = ascread(&data,&datalen);
  if (!datalen) return efmt;
  if (!w) return efmt;
  h = ascread(&data,&datalen);
  if (!datalen) return efmt;
  if (!h) return efmt;
  if (has_maxval) {
    max = ascread(&data,&datalen);
    if (!datalen) return efmt;
    if (!max) return efmt;
  }  

  /* One whitespace allowed before bitmap data */
  data++; datalen--;

  /* Check for a correct-sized data buffer */
  if (datalen < ((((w*bpp)%8) ? (w*bpp+8) : (w*bpp))/8*h))
    return efmt;

  /* Set up the bitmap */
  e = vid->bitmap_new(hbmp,w,h,vid->bpp);
  errorcheck;

#ifdef CONFIG_DITHER
  /* Start dithering */
  e = vid->dither_start(&dither, *hbmp, 0,0,0,w,h);
  errorcheck;
#endif

  /* Read in the values, convert colors, output them... */
  for (y=0;y<h;y++) {
    bit = 0;
    for (x=0;x<w;x++) {
      if (!bit)
       {
	if (bin)
	  val = *(data++);
	else
	  val = ascread(&data,&datalen);
       }

      /* Read in the RGB values */
      switch (bpp) {
      
      case 1:
	if (val&0x80)
	  r=g=b = 0;
	else
	  r=g=b = 255;
	
	/* Shift in bits, not bytes */
	val = val<<1;
	bit++;
	if (bit==8) bit = 0;
	break;

      case 8:
	/* grayscale... */
	if (max==255)
	  r=g=b = val;
	else
	  r=g=b = val * 255 / max;
	break;

      case 24:
	if (max==255)
	  r = val;
	else
	  r = val * 255 / max;

	/* Read in the other 2 bytes */
	if (bin)
	  g = *(data++);
	else
	  g = ascread(&data,&datalen);
	if (bin)
	  b = *(data++);
	else
	  b = ascread(&data,&datalen);
	if (max!=255) {
	  g = g*255/max;
	  b = b*255/max;
	}
	break;

      }

      /* Store it in the picogui bitmap */
#ifdef CONFIG_DITHER
      vid->dither_store(dither, mkcolor(r,g,b), PG_LGOP_NONE);
#else
      vid->pixel(*hbmp,x,y,VID(color_pgtohwr)(mkcolor(r,g,b)),PG_LGOP_NONE);
#endif
    }
  }

#ifdef CONFIG_DITHER
  vid->dither_finish(dither);
#endif

  return success;
}

/* The End */

