/* $Id$
 *
 * globals.c - Global object, including server resources
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
#include <pgserver/font.h>
#include <pgserver/pgstring.h>
#include <string.h>

/* Simple arrow cursor in XBM format
 */
#ifdef CONFIG_FORMAT_XBM
# define cursor_width 8
# define cursor_height 14
unsigned char const cursor_bits[] = {
  0x01,0x03,0x05,0x09,0x11,0x21,0x41,0x81,0xC1,0x21,0x2D,0x4B,0x50,0x70
};
unsigned char const cursor_mask_bits[] = {
  0x01,0x03,0x07,0x0F,0x1F,0x3F,0x7F,0xFF,0xFF,0x3F,0x3F,0x7B,0x70,0x70
};
#endif

/* Server resources */
handle res[PGRES_NUM];

g_error globals_load_strings(void);


/************************************************ Public Functions ******/

g_error globals_init(void) {
  g_error e;
  struct font_descriptor *fd;
  hwrbitmap defaultcursor_bitmap, defaultcursor_bitmask;

#ifdef DEBUG_INIT
   fprintf(stderr, "Init: globals: default font\n");
#endif

  /* Allocate default font */
  e = font_descriptor_create(&fd,NULL);
  errorcheck;
  e = mkhandle(&res[PGRES_DEFAULT_FONT],PG_TYPE_FONTDESC,-1,fd);
  errorcheck;

#ifdef DEBUG_INIT
   fprintf(stderr, "Init: globals: cursor sprite bitmaps\n");
#endif

#ifdef CONFIG_FORMAT_XBM
   /* Actually load the cursor (requires XBM image) */
   
  /* Load the default mouse cursor bitmaps */
  e = VID(bitmap_loadxbm) (&defaultcursor_bitmap,cursor_bits,
			     cursor_width,cursor_height,
			     VID(color_pgtohwr) (0xFFFFFF),
			     VID(color_pgtohwr) (0x000000));
  errorcheck;
  e = VID(bitmap_loadxbm) (&defaultcursor_bitmask,cursor_mask_bits,
			     cursor_width,cursor_height,
			     VID(color_pgtohwr) (0x000000),
			     VID(color_pgtohwr) (0xFFFFFF));
  errorcheck;

#else
   /* Fake it */
#define cursor_width  0
#define cursor_height 0
   VID(bitmap_new) (&defaultcursor_bitmap,0,0,vid->bpp);
   VID(bitmap_new) (&defaultcursor_bitmask,0,0,vid->bpp);
#endif
   
#ifdef DEBUG_INIT
   fprintf(stderr, "Init: globals: cursor sprite\n");
#endif

  /* Make handles */
  e = mkhandle(&res[PGRES_DEFAULT_CURSORBITMAP],PG_TYPE_BITMAP,-1,defaultcursor_bitmap);
  errorcheck;
  e = mkhandle(&res[PGRES_DEFAULT_CURSORBITMASK],PG_TYPE_BITMAP,-1,defaultcursor_bitmask);
  errorcheck;

#ifdef DEBUG_INIT
  fprintf(stderr, "Init: globals: strings\n");
#endif
  
  /* Default strings */
  e = globals_load_strings();
  errorcheck;

#ifdef DEBUG_INIT
   fprintf(stderr, "Init: globals: success\n");
#endif

  return success;
}


/************************************************ Internal utilities ******/

/* Load all the client-visible strings into pgstring objects,
 * and put them in the server resource table.
 */
g_error globals_load_strings(void) {
  g_error e;
  int i;

  /* Table to match resource IDs and IDs from 
   * our string table (otherwise used for errors) 
   */
  const static int strtable[][2] = {
    { PGRES_STRING_OK,         mkerror(0,1) },
    { PGRES_STRING_CANCEL,     mkerror(0,7) },
    { PGRES_STRING_YES,        mkerror(0,14) },
    { PGRES_STRING_NO,         mkerror(0,15) },
    { PGRES_STRING_SEGFAULT,   mkerror(0,16) },
    { PGRES_STRING_MATHERR,    mkerror(0,19) },
    { PGRES_STRING_PGUIERR,    mkerror(0,24) },
    { PGRES_STRING_PGUIWARN,   mkerror(0,31) },
    { PGRES_STRING_PGUIERRDLG, mkerror(0,29) },
    { PGRES_STRING_PGUICOMPAT, mkerror(0,32) },
    {0,0}
  };

  for (i=0;strtable[i][1];i++) {
    e = pgstring_wrap(&res[strtable[i][0]],PGSTR_ENCODE_ASCII,
		      errortext(strtable[i][1]));
    errorcheck;
  }

  return success;
}

/* The End */



