/* $Id$
 *
 * png.c - Use the libpng library to load PNG graphics into PicoGUI 
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

#ifdef CONFIG_PNG_LOWLEVEL
g_error png_load(hwrbitmap *hbmp, const u8 *data, u32 datalen) {
  png_structp png_ptr;
  png_infop info_ptr;
  struct datablock d;
  png_bytep *row_pointers;
  png_uint_32 width, height;
  int bit_depth,colortype,interlace;
  int x,y;
  u8 r,g,b,a;
  pgcolor c;
  png_bytep p;
  g_error e;
  png_colorp palette;
  int num_palette;
  png_colorp pp;
  png_bytep trans;
  int num_trans = 0;
#ifdef CONFIG_DITHER
  hwrdither dither;
#endif

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
	       PNG_TRANSFORM_PACKING, 
	       NULL);
  row_pointers = png_get_rows(png_ptr, info_ptr);
  png_get_IHDR(png_ptr, info_ptr, &width, &height,
	       &bit_depth, &colortype, &interlace, NULL, NULL);
  if (colortype == PNG_COLOR_TYPE_PALETTE) {
    png_get_PLTE(png_ptr, info_ptr, &palette, &num_palette);
    png_get_tRNS(png_ptr, info_ptr, &trans, &num_trans, NULL);
  }
  if (interlace != PNG_INTERLACE_NONE)
    fprintf(stderr, "png loader: OOPS... interlaced image, will b0rk\n");
  
  /* Allocate the picogui bitmap
   *
   * Important note: normally we create bitmaps at the display's
   *                 color depth, but if we have an alpha channel
   *                 we need room for ARGB colors.
   */
  if (colortype == PNG_COLOR_TYPE_GRAY_ALPHA ||
      colortype == PNG_COLOR_TYPE_RGB_ALPHA ||
      num_trans)
    e = vid->bitmap_new(hbmp,width,height,32);
  else
    e = vid->bitmap_new(hbmp,width,height,vid->bpp);
  if (iserror(e))
    png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);    
  errorcheck;

#ifdef CONFIG_DITHER
  /* Start dithering */
  e = vid->dither_start(&dither, *hbmp, 0,0,0,width,height);
  errorcheck;
#endif

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

      case PNG_COLOR_TYPE_GRAY_ALPHA:
	g = *(p++);
	a = *(p++);
	c = mkcolora(a>>1,g,g,g);
	break;

      case PNG_COLOR_TYPE_PALETTE:
	pp = &palette[ (*p) % num_palette ];
	if (*p < num_trans)
	  c = mkcolora(trans[*p]>>1, pp->red, pp->green, pp->blue);
	else if (num_trans)
	  c = mkcolora(0x7f, pp->red, pp->green, pp->blue);
	else
	  c = mkcolor(pp->red, pp->green, pp->blue);
	p++;
	break;

      case PNG_COLOR_TYPE_RGB:
	r = *(p++);
	g = *(p++);
	b = *(p++);
	c = mkcolor(r,g,b);
	break;

      case PNG_COLOR_TYPE_RGB_ALPHA:
	r = *(p++);
	g = *(p++);
	b = *(p++);
	a = *(p++);
	c = mkcolora(a>>1,r,g,b);
	break;
	
      }
#ifdef CONFIG_DITHER
      vid->dither_store(dither, c, PG_LGOP_NONE);
#else
      vid->pixel(*hbmp,x,y,VID(color_pgtohwr)(c),PG_LGOP_NONE);
#endif
    }
  }

  /* Clean up */
  png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
#ifdef CONFIG_DITHER
  vid->dither_finish(dither);
#endif
  return success;
}

#else

/* alternative implementation which lets libpng expand palletes,
 * transparencies, and any other crazyness for us
*/
g_error png_load(hwrbitmap *hbmp, const u8 *data, u32 datalen) {
  png_structp png_ptr;
  png_infop info_ptr;
  struct datablock d;
  png_bytep *row_pointers;
  png_uint_32 width, height;
  int bit_depth,colortype;
  int x,y;
  u8 r,g,b,a;
  pgcolor c;
  png_bytep p;
  g_error e;
#ifdef CONFIG_DITHER
  hwrdither dither;
#endif

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
	       PNG_TRANSFORM_PACKING |
	       PNG_TRANSFORM_EXPAND,
	       NULL);
  row_pointers = png_get_rows(png_ptr, info_ptr);
  png_get_IHDR(png_ptr, info_ptr, &width, &height,
	       &bit_depth, &colortype, NULL, NULL, NULL);
  
  /* Allocate the picogui bitmap
   *
   * Important note: normally we create bitmaps at the display's
   *                 color depth, but if we have an alpha channel
   *                 we need room for ARGB colors.
   */
  e = vid->bitmap_new(hbmp,width,height,32);
  if (iserror(e))
    png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);    
  errorcheck;

#ifdef CONFIG_DITHER
  /* Start dithering */
  e = vid->dither_start(&dither, *hbmp, 0,0,0,width,height);
  errorcheck;
#endif

  /* Transcribe it into a picogui bitmap. 
   * This method is slow, but ensures compatibility 
   */
  for (y=0;y<height;y++) {
    p = row_pointers[y];
    for (x=0;x<width;x++) {
      switch (colortype) {

      case PNG_COLOR_TYPE_GRAY:
	g = *(p++);
	c = mkcolora(0x7f,g,g,g);
	break;

      case PNG_COLOR_TYPE_GRAY_ALPHA:
	g = *(p++);
	a = *(p++);
	c = mkcolora(a>>1,g,g,g);
	break;

      case PNG_COLOR_TYPE_RGB:
	r = *(p++);
	g = *(p++);
	b = *(p++);
	c = mkcolora(0x7f,r,g,b);
	break;

      case PNG_COLOR_TYPE_RGB_ALPHA:
	r = *(p++);
	g = *(p++);
	b = *(p++);
	a = *(p++);
	c = mkcolora(a>>1,r,g,b);
	break;
	
      }
#ifdef CONFIG_DITHER
      vid->dither_store(dither, c, PG_LGOP_NONE);
#else
      vid->pixel(*hbmp,x,y,VID(color_pgtohwr)(c),PG_LGOP_NONE);
#endif
    }
  }

  /* Clean up */
  png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
#ifdef CONFIG_DITHER
  vid->dither_finish(dither);
#endif
  return success;
}
#endif CONFIG_PNG_LOWLEVEL
/* The End */

