/* $Id$
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

#ifdef DEBUG_VIDEO
#define DEBUG_FILE
#endif
#include <pgserver/debug.h>

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
#define MAXPALETTE 256

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
  const u8 *rasterdata, *linebegin;
  g_error e;
  u32 offset, compression, numcolors, *u32p;
  int x,y,shift,mask,index;
  u8 byte;
  pgcolor c, c2, colortable[MAXPALETTE];
#ifdef CONFIG_DITHER
  hwrdither dither;
#endif

  /* Load the headers. Fileheader is after "BM", infoheader is after that */
  if (datalen < FILEHEADER_LEN + INFOHEADER_LEN)
    return mkerror(PG_ERRT_BADPARAM,41);      /* Corrupt BMP header */
  fhdr = (struct bmp_fileheader *) (data + 2);
  ihdr = (struct bmp_infoheader *) (data + FILEHEADER_LEN);

  /* Important header values */
  w = LITTLE_LONG(ihdr->width);
  h = LITTLE_LONG(ihdr->height);
  bpp = LITTLE_SHORT(ihdr->bpp);
  compression = LITTLE_LONG(ihdr->compression);
  numcolors = LITTLE_LONG(ihdr->colors_used);
  offset = LITTLE_LONG(fhdr->data_offset);
  if(!numcolors)
   {
    numcolors=(offset-FILEHEADER_LEN-INFOHEADER_LEN)>>2;
    if(numcolors>1<<bpp)
      numcolors=1<<bpp;
   }

  /* Sanity checks for the header */
  if (LITTLE_LONG(ihdr->infoheader_size) != INFOHEADER_LEN)
    return mkerror(PG_ERRT_BADPARAM,41);      /* Corrupt BMP header */
  if (numcolors > MAXPALETTE)
    return mkerror(PG_ERRT_BADPARAM,41);      /* Corrupt BMP header */

  /* Supported format? */
  if (LITTLE_SHORT(ihdr->planes) != 1)
    return mkerror(PG_ERRT_BADPARAM,42);      /* Unsupported BMP format */
  switch (compression)
   {
    case 0:	/* uncompressed */
      if (bpp!=1 && bpp!=2 && bpp!=4 && bpp!=8 && bpp!=24)
	return mkerror(PG_ERRT_BADPARAM,42);      /* Unsupported BMP format */
      break;
    case 1:	/* 8-bit RLE */
      if(bpp!=8)
	return mkerror(PG_ERRT_BADPARAM,42);      /* Unsupported BMP format */
      break;
    case 2:	/* 4-bit RLE */
      if(bpp!=4)
	return mkerror(PG_ERRT_BADPARAM,42);      /* Unsupported BMP format */
      break;
    default:
      return mkerror(PG_ERRT_BADPARAM,42);      /* Unsupported BMP format */
   }
  
  /* Load palette */
  DBG("%dx%d, %d bpp, compression %d, %d/%d colors\n", w, h, bpp,
      compression, numcolors, LITTLE_LONG(ihdr->colors_important));
  if (FILEHEADER_LEN+INFOHEADER_LEN+4*numcolors>datalen)
    return mkerror(PG_ERRT_BADPARAM,41);	/* Corrupt BMP header */
  u32p = (u32 *) (data + FILEHEADER_LEN + INFOHEADER_LEN);
  for(index=0;index<numcolors;index++)
   {
    colortable[index]=LITTLE_LONG(u32p[index]);
    DBG("Color %d: %06x\n", index, colortable[index]);
   }

  /* Find the raster data */
  if(!compression)
   {
    if (offset + ((w*h*bpp)>>3) > datalen)
      return mkerror(PG_ERRT_BADPARAM,41);    /* Corrupt BMP header */
   }
  else
    if (offset + ihdr->image_size > datalen)
      return mkerror(PG_ERRT_BADPARAM,41);    /* Corrupt BMP header */
  rasterdata = data + offset;

  /* Ok so far, so allocate the bitmap.
   * IMPORTANT: If any errors occur after this, we have to
   * free the bitmap. It hasn't been assigned a handle yet, so failing
   * to free the bitmap would introduce a memory leak!
   */
  e = (*vid->bitmap_new) (hbmp,w,h,vid->bpp);
  errorcheck;

#ifdef CONFIG_DITHER
  /* Start dithering */
  e = vid->dither_start(&dither, *hbmp, 1,0,0,w,h);
  errorcheck;
#endif

  /* If we're converting from < 8bpp, make a mask */
  mask = (1<<bpp)-1;

  switch (compression) {
    case 0:	/* Uncompressed */
      /* Loop through each line of raster data, from the bottom to the top */
      for (y=h-1;y>=0;y--) {
    
	/* Transcribe the raster data into the hwrbitmap. This code
	 * will depend on the format used to store the raster data.
	 */
    
	for (shift=-1,x=0,linebegin=rasterdata;x<w;x++) {
	
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
		c = colortable[index];
	      break;

	    /* 8 bit palettized */
	    case 8:
	      index = *(rasterdata++);
	      if (index < numcolors)
		c = colortable[index];
	      break;

	    /* 24 bit true color */
	    case 24:
	      c  =  *(rasterdata++);
	      c |= (*(rasterdata++)) << 8;
	      c |= (*(rasterdata++)) << 16;
	      break;
	  }
	
#ifdef CONFIG_DITHER
	  vid->dither_store(dither, c, PG_LGOP_NONE);
#else
	  /* Convert and store c. This method is not the fastest, but
	   * it will always work and it's easy. If more speed is needed here,
	   * we could use a pixel-pushing loop like the PNM driver does.
	   */
	  (*vid->pixel) (*hbmp,x,y,(*vid->color_pgtohwr)(c),PG_LGOP_NONE);
#endif
	}
	/* Pad to a 32-bit boundary */
	x = (rasterdata-linebegin) & 0x03;
	if (x)
	  rasterdata += 4-x;
      }
      break;
    
    case 1:	/* 8-bit RLE */
    case 2:	/* 4-bit RLE */
      /*datalen=offset+ihdr->image_size;*/
      c=colortable[0];
      for(y=0;y<h;y++)
	for(x=0;x<w;x++) {
	  /* FIXME: We can't do dithering for RLE images yet */
	  (*vid->pixel) (*hbmp,x,y,(*vid->color_pgtohwr)(c),PG_LGOP_NONE);
	}
      x=0;
      y=h-1;
      while(rasterdata+1<data+datalen)
       {
	unsigned char rle_n, rle_c;
	rle_n=*(rasterdata++);
	rle_c=*(rasterdata++);
	DBG("RLE: %d,%d %#02x %#02x\n", x, y, rle_n, rle_c);
	if(rle_n)	/* RLE pixels */
	 {
	  if(compression==1)	/* 8-bit */
	    c = c2 = colortable[rle_c];
	  else			/* 4-bit */
	   {
	    c = colortable[rle_c>>4];
	    c2 = colortable[rle_c&0xf];
	   }
	  while(rle_n--)
	   {
	    if(x==w)
	     {
	      x=0;
	      y--;
	     }
	    if(y<0)
	      return mkerror(PG_ERRT_BADPARAM,41);    /* Too few lines */
	    (*vid->pixel) (*hbmp,x++,y,(*vid->color_pgtohwr)(rle_n&1?c2:c),PG_LGOP_NONE);
	   }
	 }
	else
	  switch(rle_c)
	   {
	    case 0:	/* end of line */
	      y--;
	      if(y<-1)
		return mkerror(PG_ERRT_BADPARAM,41); /* Too few lines */
	      x=0;
	      break;
	    case 1:	/* end of bitmap ... */
	      return success;
	    case 2:	/* delta */
	      if(rasterdata+2>data+datalen)
		return mkerror(PG_ERRT_BADPARAM,41); /* RLE data cut short */
	      x+=*(rasterdata++);
	      if(x>w)
		return mkerror(PG_ERRT_BADPARAM,41); /* Too few columns */
	      y-=*(rasterdata++);
	      if(y<0)
		return mkerror(PG_ERRT_BADPARAM,41); /* Too few lines */
	      break;
	    default:	/* rle_c uncompressed pixels */
	      if(rasterdata+((rle_c+1)&0x1fe)>data+datalen)
		return mkerror(PG_ERRT_BADPARAM,41); /* RLE data cut short */
	      rle_n=rle_c&1;		/* for alignment */
	      if(compression==2)	/* 4-bit */
		rle_n^=(rle_c>>1)&1;
	      c2=0;
	      while(rle_c--)
	       {
		if(x==w)
		 {
		  x=0;
		  y--;
		 }
		if(y<0)
		  return mkerror(PG_ERRT_BADPARAM,41); /* Too few lines */
		if(!c2)
		 {
		  index=*(rasterdata++);	/* read another byte */
		  c2=compression;		/* pixels per byte */
		 }
		switch(compression+c2)
		 {
		  case 2:	/* 1 8-bit pixel */
		    c = colortable[index];
		    break;
		  case 3:	/* 2nd 4-bit pixel */
		    c = colortable[index&0xf];
		    break;
		  case 4:	/* 1st 4-bit pixel */
		    c = colortable[index>>4];
		    break;
		 }
		(*vid->pixel) (*hbmp,x++,y,(*vid->color_pgtohwr)(c),
			       PG_LGOP_NONE);
		c2--;
	       }
	      rasterdata+=rle_n;	/* alignment */
	      break;
	   }
       }
      return mkerror(PG_ERRT_BADPARAM,41);      /* RLE data cut short */
  }
      
#ifdef CONFIG_DITHER
  vid->dither_finish(dither);
#endif

  return success;
}

/* The End */
