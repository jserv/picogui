/* $Id$
 *
 * dvbl_color.c - This file is part of the Default Video Base Library,
 *                providing the basic video functionality in picogui but
 *                easily overridden by drivers.
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
#include <pgserver/video.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#ifdef DEBUG_VIDEO
#define DEBUG_FILE
#endif
#include <pgserver/debug.h>

#ifdef CONFIG_PAL8_CUSTOM
pgcolor palette8_custom[256];

#ifdef CONFIG_PAL8_VISIBONE2
extern pgcolor visibone2_palette[256];
#endif

#ifdef CONFIG_PAL8_LOOKUP_32K
u8 palette_hash[1 << 15];
#endif

#ifdef CONFIG_PAL8_LOOKUP_256K
u8 palette_hash[1 << 18];
#endif

/* qsort comparison function to sort by luminance */
int compare_luminance(const void *a, const void *b) {
  pgcolor color_a = *(pgcolor *)a;
  pgcolor color_b = *(pgcolor *)b;
  int lum_a = getred(color_a)*30 + getgreen(color_a)*59 + getblue(color_a)*11;
  int lum_b = getred(color_b)*30 + getgreen(color_b)*59 + getblue(color_b)*11;
  if (lum_a < lum_b)
    return -1;
  if (lum_a > lum_b)
    return 1;
  return 0;
}

/* Look up a pgcolor in the palette. Really slow... */
hwrcolor palette_lookup(pgcolor c) {
  /* The downside of custom palettes is that we have to spend a good bit of
   * time looking up the color. Make this a little faster by caching the
   * most recently looked up color, since in image conversion there are usually
   * areas of equal color.
   */
  
  static pgcolor cached_color = 0xFFFFFFFF;  /* An invalid value that we won't get a cache hit on */
  static u8 cached_index = 0;
  int i;
  int distance, d;
  int r,g,b,rd,gd,bd;
  pgcolor pcolor;
  
  if (cached_color == c)
    return cached_index;
  
  /* Since our palette is sorted by luminance, we can handle black and white easily */
  if (c == 0xFFFFFF)
    return 255;
  if (c == 0)
    return 0;
  
  /* Now search for the closest color in RGB space */
  
  distance = 195076;  /* This is one greater than the distance between black and white */
  cached_color = c;
  r = getred(c);
  g = getgreen(c);
  b = getblue(c);
  for (i=0;i<256;i++) {
    pcolor = palette8_custom[i];
    rd = r - getred(pcolor);
    gd = g - getgreen(pcolor);
    bd = b - getblue(pcolor);
    d  = rd*rd + gd*gd + bd*bd;
    if (d < distance) {
      distance = d;
      cached_index = i;
    }
  }
  return cached_index;
}

g_error load_custom_palette(const char *name) {
  int i;
  FILE *f;
  char linebuffer[256];
  char *p;
  int r,g,b;

  if (name) {
    /* Load a gimp-format palette from file */
   
    f = fopen(name,"r");
    if (!f)
      return mkerror(PG_ERRT_IO,110);    /* Can't open palette */

    /* Zero unused entries */
    memset(palette8_custom,0,sizeof(palette8_custom));
    i = 0;

    while (fgets(linebuffer,sizeof(linebuffer)-1,f)) {
      linebuffer[sizeof(linebuffer)-1] = 0;

      /* Strip comments out */
      p = strchr(linebuffer,'#');
      if (p)
	*p = 0;

      /* Strip leading whitespace */
      p = linebuffer;
      while (isspace(*p))
	p++;

      /* If there's anything else, read in the palette entry */
      if (*p) {
	sscanf(p, "%d %d %d", &r,&g,&b);
	palette8_custom[(i++)&0xFF] = mkcolor(r,g,b);
      }
    }
    fclose(f);
  }
  else {
    /* Load a default palette.
     * Grayscale is an easy fallback, but we can optionally include
     * a copy of the visibone2 palette from Gimp
     */
    
#ifdef CONFIG_PAL8_VISIBONE2
    memcpy(palette8_custom,visibone2_palette,sizeof(palette8_custom));
#else
    for (i=0;i<256;i++)
      palette8_custom[i] = mkcolor(i,i,i);
#endif
  }    

  /* Now sort the palette by luminance so the AND/OR effects and such work */
  qsort(palette8_custom, 256, sizeof(pgcolor), compare_luminance);

  /* Build the palette hash table if we're using one.
   * Note the extra care in bitshifting so that we really do get pure white,
   * rather than always truncating a bit or three
   */

#ifdef CONFIG_PAL8_LOOKUP_32K
  for (i=0;i<sizeof(palette_hash)/sizeof(palette_hash[0]);i++) {
    int r = i >> 10;
    int g = (i >> 5 ) & 0x1F;
    int b = i & 0x1F;
    palette_hash[i] = palette_lookup(mkcolor( (r >> 2) | (r << 3),
					      (g >> 2) | (g << 3),
					      (b >> 2) | (b << 3) ));
  }
#endif

#ifdef CONFIG_PAL8_LOOKUP_256K
  for (i=0;i<sizeof(palette_hash)/sizeof(palette_hash[0]);i++) {
    int r = i >> 12;
    int g = (i >> 6 ) & 0x3F;
    int b = i & 0x3F;
    palette_hash[i] = palette_lookup(mkcolor( (r >> 4) | (r << 2),
					      (g >> 4) | (g << 2),
					      (b >> 4) | (b << 2) ));
  }
#endif

  return success;
}
#endif /* CONFIG_PAL8_CUSTOM */


hwrcolor def_color_pgtohwr(pgcolor c) {
  if (c & PGCF_ALPHA) {
    /* ARGB conversion, just premultiply the RGB color */
    return mkcolora( getalpha(c),
		     (getred(c)   * getalpha(c)) >> 7,
		     (getgreen(c) * getalpha(c)) >> 7,
		     (getblue(c)  * getalpha(c)) >> 7
		     );
  }
  else if (vid->bpp==0) {
    return c;
  }
  else if (vid->bpp==1) {
    /* Black and white, tweaked for better contrast */
    return (getred(c)+getgreen(c)+getblue(c)) >= 382;
  }
  else if(vid->bpp<8) {
    /* grayscale */
    return (getred(c)+getgreen(c)+getblue(c)) * ((1<<vid->bpp)-1) / 765;
  }
#ifdef CONFIG_PAL8_233
  else if (vid->bpp==8) {
    /* 2-3-3 color */
    return ((getred(c) & 0xC0) |
	    ((getgreen(c) >> 2) & 0x38) |
	    ((getblue(c) >> 5)));
  }
#endif
#ifdef CONFIG_PAL8_222
  else if (vid->bpp==8) {
    /* 2-2-2 color */
    return (((getred(c) >> 2) & 0x30) |
	    ((getgreen(c) >> 4) & 0x0C) |
	    ((getblue(c) >> 6)));
  }
#endif
#ifdef CONFIG_PAL8_CUSTOM
  else if (vid->bpp==8) {
#ifdef CONFIG_PAL8_LOOKUP_EXACT
    /* Do a palette lookup each time.. slow */
    return palette_lookup(c);
#endif
#ifdef CONFIG_PAL8_LOOKUP_32K
    /* Hash it down to a 5-5-5 color and look it up */
    return palette_hash[(((getred(c)   << 7) & 0x7C00)  |
			 ((getgreen(c) << 2) & 0x03E0)  |
			 ((getblue(c)  >> 3) & 0x001F)) ];
#endif
#ifdef CONFIG_PAL8_LOOKUP_256K
    /* Hash it down to a 6-6-6 color and look it up */
    return palette_hash[(((getred(c)   << 10) & 0x03F000)  |
			 ((getgreen(c) << 4) & 0x000FC0)  |
			 ((getblue(c)  >> 2) & 0x00003F)) ];
#endif
  }
#endif
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

    int alpha = getalpha(c);
    if (alpha)
      return mkcolora( alpha,
		       (getred(c)   << 7) / alpha,
		       (getgreen(c) << 7) / alpha,
		       (getblue(c)  << 7) / alpha
		       );
    return mkcolora(0,0,0,0);
  }
  else if (vid->bpp==0) {
    return c;
  }
  else if (vid->bpp==1) {
    return c ? 0xFFFFFF : 0x000000;  
  }
  else if (vid->bpp<8) {
    /* grayscale */
    unsigned char gray = c * 255 / ((1<<vid->bpp)-1);
    return mkcolor(gray,gray,gray);
  }
#ifdef CONFIG_PAL8_233
  else if (vid->bpp==8) {
    /* 2-3-3 color */  
    u8 r = c&0xC0, g = (c&0x38)<<2, b = c<<5;
    return mkcolor(r | (r>>2) | (r>>4) | (r>>6),
		   g | (g>>3) | (g>>6),
		   b | (b>>3) | (b>>6));
  }
#endif
#ifdef CONFIG_PAL8_222
  else if (vid->bpp==8) {
    /* 2-2-2 color */  
    u8 r = (c&0x30)<<2, g = (c&0x0C)<<4, b = c<<6;
    return mkcolor(r | (r>>2) | (r>>4) | (r>>6),
		   g | (g>>2) | (g>>2) | (g>>2),
		   b | (b>>2) | (b>>2) | (b>>2));
  }
#endif
#ifdef CONFIG_PAL8_CUSTOM
  else if (vid->bpp==8) {
    return palette8_custom[c&0xFF];
  }
#endif
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
