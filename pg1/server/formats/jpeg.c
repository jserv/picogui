/* $Id$
 *
 * jpeg.c - Functions to convert any of the jpeg formats 
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

#include <stdio.h>
#include <string.h>
#include <pgserver/common.h>
#include <pgserver/video.h>
#include <jpeglib.h>
#include <jerror.h>

/* this is global so we can use it for the source too */
typedef struct jpeg_data_s {
  unsigned char *buf;
  int   size;
  unsigned char *lastptr;
  int   copied_size;
} jpeg_data_t;

static jpeg_data_t g_jpeg_data;

typedef struct {
  struct jpeg_source_mgr pub;	/* public fields */
  JOCTET *buffer;		/* start of buffer */
  bool   start_of_file;
} pgui_jpeg_source_mgr;

typedef pgui_jpeg_source_mgr * pgui_jpeg_src_ptr;

/*    struct jpeg_decompress_struct cinfo; */
/*    struct jpeg_error_mgr jerr; */


#define INPUT_BUF_SIZE   4096 /* choose an efficiently fread'able size */

void pgui_jpeg_init_source (j_decompress_ptr cinfo)
{
  pgui_jpeg_src_ptr src = (pgui_jpeg_src_ptr) cinfo->src;

  /* We reset the empty-input-file flag for each image,
   * but we don't clear the input buffer.
   * This is correct behavior for reading a series of images from one source.
   */
  src->start_of_file = TRUE;
  g_jpeg_data.copied_size=0;
  g_jpeg_data.lastptr=g_jpeg_data.buf;
}


boolean pgui_jpeg_fill_input_buffer (j_decompress_ptr cinfo)
{
  pgui_jpeg_src_ptr src = (pgui_jpeg_src_ptr) cinfo->src;
  size_t nbytes;
  int size=INPUT_BUF_SIZE;

  // make sure we don't read past "size".. since we kept that in the jpeg_data structure,
  // artifically increment nbytes to look like it incremented the right size. (BUF_SIZE,
  // or remaining bytes left from jpeg_data->size
  size = ((g_jpeg_data.size-g_jpeg_data.copied_size) > INPUT_BUF_SIZE) ?  
    INPUT_BUF_SIZE : 
    (g_jpeg_data.size - g_jpeg_data.copied_size);

  memcpy(src->buffer, g_jpeg_data.lastptr, size);
  if(g_jpeg_data.copied_size < (g_jpeg_data.size - INPUT_BUF_SIZE)) { 
    nbytes=INPUT_BUF_SIZE;
    g_jpeg_data.lastptr+=nbytes;
  } else {
    nbytes=(g_jpeg_data.size - g_jpeg_data.copied_size);
  }

  g_jpeg_data.copied_size+=nbytes;
 
  // if there is no data, and we have just started, mark an exit
  if (nbytes <= 0) {
    if (src->start_of_file){	/* Treat empty input file as fatal error */
      printf("error: no data!\n");
      WARNMS(cinfo, JERR_INPUT_EMPTY); 
      return FALSE;

    }
    WARNMS(cinfo, JWRN_JPEG_EOF);
    /* Insert a fake EOI marker */
    src->buffer[0] = (JOCTET) 0xFF;
    src->buffer[1] = (JOCTET) JPEG_EOI;
    nbytes = 2;
  }

  src->pub.next_input_byte = src->buffer;
  src->pub.bytes_in_buffer = nbytes;
  src->start_of_file = FALSE;

  return TRUE;
}


void pgui_jpeg_skip_input_data (j_decompress_ptr cinfo, long num_bytes)
{
  pgui_jpeg_src_ptr src = (pgui_jpeg_src_ptr) cinfo->src;

  /* Just a dumb implementation for now.  Could use fseek() except
   * it doesn't work on pipes.  Not clear that being smart is worth
   * any trouble anyway --- large skips are infrequent.
   */
  if (num_bytes > 0) {
    while (num_bytes > (s32) src->pub.bytes_in_buffer) {
      num_bytes -= (s32) src->pub.bytes_in_buffer;
      (void) pgui_jpeg_fill_input_buffer(cinfo);
      /* note we assume that fill_input_buffer will never return FALSE,
       * so suspension need not be handled.
       */
    }
    src->pub.next_input_byte += (size_t) num_bytes;
    src->pub.bytes_in_buffer -= (size_t) num_bytes;
  }
}


void pgui_jpeg_term_source (j_decompress_ptr cinfo)
{
  /* no work necessary here */
}

// source memory used for jpeg.
void jpeg_mem_src (j_decompress_ptr cinfo, unsigned char  *inbuf, int size)
{
  pgui_jpeg_src_ptr src;

  /* The source object and input buffer are made permanent so that a series
   * of JPEG images can be read from the same file by calling jpeg_stdio_src
   * only before the first one.  (If we discarded the buffer at the end of
   * one image, we'd likely lose the start of the next one.)
   * This makes it unsafe to use this manager and a different source
   * manager serially with the same JPEG object.  Caveat programmer.
   */
  if (cinfo->src == NULL) {	/* first time for this JPEG object? */
    cinfo->src = (struct jpeg_source_mgr *)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
				  sizeof(pgui_jpeg_source_mgr));
    src = (pgui_jpeg_src_ptr) cinfo->src;
    src->buffer = (JOCTET *)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
				  INPUT_BUF_SIZE * sizeof(JOCTET));
  }

  src = (pgui_jpeg_src_ptr) cinfo->src;
  g_jpeg_data.buf = inbuf;
  g_jpeg_data.size = size;
  src->pub.init_source = pgui_jpeg_init_source;
  src->pub.fill_input_buffer = pgui_jpeg_fill_input_buffer;
  src->pub.skip_input_data = pgui_jpeg_skip_input_data; src->pub.resync_to_restart = jpeg_resync_to_restart; /* use default method */
  src->pub.term_source = pgui_jpeg_term_source;
  src->pub.bytes_in_buffer = 0; /* forces fill_input_buffer on first read */
  src->pub.next_input_byte = NULL; /* until buffer loaded */

}



/**************************** Detect */

bool jpeg_detect(const u8 *data, u32 datalen) {
  /* 
   * According to CCITT Rec. T.81, Annex B, the standard JPEG 
   * interchange format can be identified by a start-of-image 
   * marker (SOI: 0xFF 0xD8) at the beginning of the file, an 
   * end-of-image marker (EOI: 0xFF 0xD9) at the end of the file, 
   * and various marker segments in the middle, depending on the 
   * encoding.
   *
   * A marker segment is a 2-byte value always starting with 0xFF,
   * possibly followed by a segment length and parameters, but not
   * always.
   *
   * Most JPEG-encoded images use the JPEG File Interchange Format
   * (JFIF), which is a subset of the standard JPEG interchange format,
   * but not all.
   * To keep the  detection simple yet not totally trivial, and allow
   * detection of JPEG data not following the JFIF format (i.e.
   * not using an APPx (0xE?) marker right after SOI), we will check:
   *
   *  - 0xFF 0xD8 0xFF   at the beginning
   *  - 0xFF 0xD9        at the end
   */

  char jpeg_start [3] = { 0xFF, 0xD8, 0xFF };
  char jpeg_end   [2] = { 0xFF, 0xD9 };

  if (datalen < 3) return 0;

  if ( (strncmp (jpeg_start, data,               3) == 0) &&
       (strncmp (jpeg_end,   data + datalen - 2, 2) == 0) ) {
    return 1;
  } else {
    return 0;
  }
}


g_error jpeg_load(hwrbitmap *hbmp, const u8 *data, u32 datalen) {
  /* Convert from any of the pbmplus formats in binary or ascii */
  struct stdbitmap **bmp = (struct stdbitmap **) hbmp;
  g_error e;
  int i,r,g,b,x,y;

  int pixels;
  int bytes;
  int scanline_bytes;
  struct jpeg_decompress_struct cinfo; 
  struct jpeg_error_mgr jerr; 
  JSAMPARRAY buffer;		/* Output row buffer */
#ifdef CONFIG_DITHER
  hwrdither dither;
#endif

  g_error efmt = mkerror(PG_ERRT_BADPARAM,48);

  if (!datalen) return efmt;

  /* get jpeg info */
  cinfo.err = jpeg_std_error(&jerr); 
  jpeg_create_decompress(&cinfo); 
  jpeg_mem_src(&cinfo, (unsigned char *) data, datalen);  
  jpeg_read_header(&cinfo, TRUE); 
  jpeg_start_decompress(&cinfo);

  /* determine memory requirements */
  pixels = cinfo.output_width * cinfo.output_height;
  bytes  = pixels * cinfo.output_components;
  scanline_bytes = cinfo.output_components * cinfo.output_width;

#if DEBUG
  printf(__FUNCTION__ "() image is %dx%dx%d (%d bytes), colorspace=%d\n",	   cinfo.output_width,
	 cinfo.output_height,
	 cinfo.output_components,
	 bytes,
	 cinfo.out_color_components);
#endif

  /* Set up the bitmap */
  e = vid->bitmap_new((hwrbitmap *)bmp,cinfo.output_width,
		      cinfo.output_height,vid->bpp);
  errorcheck;

#ifdef CONFIG_DITHER
  /* Start dithering */
  e = vid->dither_start(&dither, *bmp, 0,0,0,cinfo.output_width,cinfo.output_height);
  errorcheck;
#endif

  /* Make a one-row-high sample array that will go away when done with image */
  buffer = (*cinfo.mem->alloc_sarray)
		((j_common_ptr) &cinfo, JPOOL_IMAGE, scanline_bytes, 1);

  y = 0;
  while (cinfo.output_scanline <   cinfo.output_height  ) 
  {

    jpeg_read_scanlines(&cinfo, buffer, 1);

    /* process scanline */
    for (i=0,x=0;i<scanline_bytes;x++) {

      switch(cinfo.out_color_components) {
      case 3:
	r = (*buffer)[i++];
	g = (*buffer)[i++];
	b = (*buffer)[i++];
	break;
      default:
	r = g = b = (*buffer)[i++];
      }

#ifdef CONFIG_DITHER
      vid->dither_store(dither, mkcolor(r,g,b), PG_LGOP_NONE);
#else
      vid->pixel(*bmp,x,y,vid->color_pgtohwr(mkcolor(r,g,b)),PG_LGOP_NONE);
#endif
    }
    y++;
  }
  jpeg_finish_decompress(&cinfo);
  jpeg_destroy_decompress(&cinfo);
#ifdef CONFIG_DITHER
  vid->dither_finish(dither);
#endif

  return success;
}

/* The End */

