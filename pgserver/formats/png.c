/* $Id: png.c,v 1.1 2002/01/10 15:22:12 micahjd Exp $
 *
 * png.c - Use the libpng library to load PNG graphics into PicoGUI 
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

#include <pgserver/common.h>
#include <pgserver/video.h>
#include <png.h>

/**************************** Detect */

/* Detection is easy with libpng :) 
 */
bool png_detect(const u8 *data, u32 datalen) {
  return (datalen > 4) && !png_sig_cmp((png_bytep) data, (png_size_t) 0, 4);
}

/**************************** Load */

struct datablock {
  const u8 *data;
  u32 length;
};

/* Custom read function to read from a block of memory
 */
void png_user_read_data(png_structp png_ptr, png_bytep data, png_uint_32 length) {
  struct datablock *d = (struct datablock *) png_get_io_ptr(png_ptr);
 
  if (length > d->length) {
    /* This shouldn't normally happen, but we should probably Not Crash */
    length = d->length;
  }

  memcpy(data, d->data, length);
  
  d->data += length;
  d->length -= length;
}

g_error png_load(hwrbitmap *hbmp, const u8 *data, u32 datalen) {
  png_structp png_ptr;
  png_infop info_ptr;
  struct datablock d;
  png_bytep *row_pointers;
  png_uint_32 width, height;
  int bit_depth,colortype;
  int x,y;
  u8 r,g,b;
  pgcolor c;
  png_bytep p;
  g_error e;
  png_colorp palette;
  int num_palette;
  png_colorp pp;

  png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png_ptr)
    return mkerror(PG_ERRT_IO, 68);   /* Error initializing libpng */
  
  info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
    return mkerror(PG_ERRT_IO, 68);   /* Error initializing libpng */
  }

  /* Set libpng error handler */
  if (setjmp(png_jmpbuf(png_ptr))) {
    png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
    return mkerror(PG_ERRT_IO, 71);   /* Error reading PNG */
  }

  /* Set libpng read handler */
  d.data = data;
  d.length = datalen;
  png_set_read_fn(png_ptr, &d, (png_rw_ptr) &png_user_read_data);

  /* Read the png into memory */
  png_read_png(png_ptr, info_ptr, 
	       PNG_TRANSFORM_STRIP_16 | 
	       PNG_TRANSFORM_STRIP_ALPHA |
	       PNG_TRANSFORM_PACKING, 
	       NULL);
  row_pointers = png_get_rows(png_ptr, info_ptr);
  png_get_IHDR(png_ptr, info_ptr, &width, &height,
	       &bit_depth, &colortype, NULL, NULL, NULL);
  if (colortype == PNG_COLOR_TYPE_PALETTE)
    png_get_PLTE(png_ptr, info_ptr, &palette, &num_palette);
  
  /* Allocate the picogui bitmap */
  e = vid->bitmap_new(hbmp,width,height);
  if (iserror(e))
    png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);    
  errorcheck;

  /* Transcribe it into a picogui bitmap. 
   * This method is slow, but ensures compatibility 
   */
  for (y=0;y<height;y++) {
    p = row_pointers[y];
    for (x=0;x<width;x++) {
      switch (colortype) {

      case PNG_COLOR_TYPE_GRAY:
	g = *(p++);
	c = mkcolor(g,g,g);
	break;

      case PNG_COLOR_TYPE_PALETTE:
	pp = &palette[ (*(p++)) % num_palette ];
	c = mkcolor(pp->red, pp->green, pp->blue);
	break;

      case PNG_COLOR_TYPE_RGB:
	r = *(p++);
	g = *(p++);
	b = *(p++);
	c = mkcolor(r,g,b);
	break;
	
      }
      vid->pixel(*hbmp,x,y,VID(color_pgtohwr)(c),PG_LGOP_NONE);
    }
  }

  /* Clean up */
  png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
  return success;
}

/* The End */

