/* $Id: bmp.c,v 1.3 2001/09/18 17:14:11 micahjd Exp $
 *
 * bmp.c - Functions to detect and load files compatible with the Windows BMP
 *         file format. This format is good for palettized images and/or
 *         images with RLE compression.
 *
 *         This program tries to be compatible with the BMP format as
 *         described by:
 *           http://www.daubnet.com/formats/BMP.html
 *         With some exceptions... The 24-bit format described in the
 *         document is incorrect. Instead of 00RRGGBB as the document
 *         specifies, the actual format is BBGGRR.
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

/**************************** Format Spec */

/* Small macros to convert little-endian values to the CPU's format */
#define LITTLE_SHORT(x)  ( ((u16)( (u8*)(&(x)))[0]     ) |\
			   ((u16)(((u8*)(&(x)))[1]<<8 )) )
#define LITTLE_LONG(x)   ( ((u32)( (u8*)(&(x)))[0]     ) |\
			   ((u32)(((u8*)(&(x)))[1]<<8 )) |\
                           ((u32)(((u8*)(&(x)))[2]<<16)) |\
			   ((u32)(((u8*)(&(x)))[3]<<24)) )

/* blech. These have to be 2 separate structures because the lousy
 * programmers at M$ didn't pad them to word boundaries...
 * If we don't do it like this, non-Intel platforms will probably screw up.
 */

/* All the 16- and 32-bit values are in little-endian */
#define FILEHEADER_LEN 14
struct bmp_fileheader {
  u32 filesize;
  u32 reserved;
  u32 data_offset;     /* File offset to raster data */

  /* Info header follows */
};
#define INFOHEADER_LEN 40
struct bmp_infoheader {
  u32 infoheader_size;
  u32 width;
  u32 height;
  u16 planes;
  u16 bpp;
  u32 compression;      /* 0 = none, 1 = 8bit RLE, 2 = 4bit RLE */
  u32 image_size;       /* Size of compressed image, or 0 */
  u32 xppm,yppm;        /* Pixels per meter */
  u32 colors_used;      /* Number of colors actually used */
  u32 colors_important; /* Number of important colors */
  
  /* Color table follows */
};

/**************************** Detect */

/* Make sure it's big enough to have the right header, and check the
 * header for the "BM" magic number
 */
bool bmp_detect(const u8 *data, u32 datalen) {
  return (datalen > FILEHEADER_LEN + INFOHEADER_LEN) && 
    (data[0] == 'B') && (data[1] == 'M');
}

/**************************** Load */

g_error bmp_load(hwrbitmap *hbmp, const u8 *data, u32 datalen) {
  struct bmp_fileheader *fhdr;
  struct bmp_infoheader *ihdr;
  int w,h,bpp;
  const u8 *rasterdata;
  g_error e;
  u32 offset,compression,numcolors;
  int x,y,shift,mask,index;
  u8 byte;
  pgcolor c;
  const u8 *linebegin;
  u32 *colortable;

  /* Load the headers. Fileheader is after "BM", infoheader is after that */
  if (datalen < FILEHEADER_LEN + INFOHEADER_LEN)
    return mkerror(PG_ERRT_BADPARAM,41);      /* Corrupt BMP header */
  fhdr = (struct bmp_fileheader *) (data + 2);
  ihdr = (struct bmp_infoheader *) (data + FILEHEADER_LEN);
  colortable = (u32 *) (data + FILEHEADER_LEN + INFOHEADER_LEN);

  /* Important header values */
  w = LITTLE_LONG(ihdr->width);
  h = LITTLE_LONG(ihdr->height);
  bpp = LITTLE_SHORT(ihdr->bpp);
  compression = LITTLE_LONG(ihdr->compression);
  numcolors = LITTLE_LONG(ihdr->colors_used);

  /* Sanity checks for the header */
  if (LITTLE_LONG(ihdr->infoheader_size) != INFOHEADER_LEN)
    return mkerror(PG_ERRT_BADPARAM,41);      /* Corrupt BMP header */
  if (numcolors > 256)
    return mkerror(PG_ERRT_BADPARAM,41);      /* Corrupt BMP header */

  /* Supported format? */
  if (LITTLE_SHORT(ihdr->planes) != 1)
    return mkerror(PG_ERRT_BADPARAM,42);      /* Unsupported BMP format */
  if (compression > 2)
    return mkerror(PG_ERRT_BADPARAM,42);      /* Unsupported BMP format */
  if (bpp!=1 && bpp!=2 && bpp!=4 && bpp!=8 && bpp!=24)
    return mkerror(PG_ERRT_BADPARAM,42);      /* Unsupported BMP format */
  
  /* Find the raster data */
  offset = LITTLE_LONG(fhdr->data_offset);
  if (offset + ((w*h*bpp)>>3) > datalen)
    return mkerror(PG_ERRT_BADPARAM,41);      /* Corrupt BMP header */
  rasterdata = data + offset;

  /* Ok so far, so allocate the bitmap.
   * IMPORTANT: If any errors occur after this, we have to
   * free the bitmap. It hasn't been assigned a handle yet, so failing
   * to free the bitmap would introduce a memory leak!
   */
  e = (*vid->bitmap_new) (hbmp,w,h);
  errorcheck;

  /* If we're converting from < 8bpp, make a mask */
  mask = (1<<bpp)-1;
  shift = -1;

  /* Loop through each line of raster data, from the bottom to the top */
  for (y=h-1;y>=0;y--) {
    
    /* Transcribe the raster data into the hwrbitmap. This code
     * will depend on the format used to store the raster data.
     */
    
    switch (compression) {
      
    case 0:      /* Uncompressed */

      for (x=0,linebegin=rasterdata;x<w;x++) {
	
	/* Format here depends on color depth. Load the next
	 * pixel into 'c' as a pgcolor */
	switch (bpp) {

	  /* < 8bpp palettized */
	case 1:
	case 2:
	case 4:
	  if (shift < 0) {
	    shift = 8-bpp;
	    byte = *(rasterdata++);
	  }
	  index = (byte>>shift) & mask;
	  shift -= bpp;
	  if (index < numcolors)
	    c = LITTLE_LONG(colortable[index]);
	  break;

	  /* 8 bit palettized */
	case 8:
	  index = *(rasterdata++);
	  if (index < numcolors)
	    c = LITTLE_LONG(colortable[index]);
	  break;

	  /* 24 bit true color */
	case 24:
	  c  =  *(rasterdata++);
	  c |= (*(rasterdata++)) << 8;
	  c |= (*(rasterdata++)) << 16;
	  break;
	}
	
	/* Convert and store c. This method is not the fastest, but
	 * it will always work and it's easy. If more speed is needed here,
	 * we could use a pixel-pushing loop like the PNM driver does.
	 */
	(*vid->pixel) (*hbmp,x,y,(*vid->color_pgtohwr)(c),PG_LGOP_NONE);
      }
      break;

    default:
      
      /* FIXME: Implement compressed BMP files */

    }      
      
    /* Pad to a 32-bit boundary */
    x = (rasterdata-linebegin) & 0x03;
    if (x)
      rasterdata += 4-x;
  }
    
  return sucess;
}

/* The End */



