/* $Id: g_error.c,v 1.14 2001/01/29 00:22:33 micahjd Exp $
 *
 * g_error.h - Defines a format for errors
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000 Micah Dowty <micahjd@users.sourceforge.net>
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

#include <pgserver/g_error.h>

#ifdef DEBUG_ANY
/* Extra includes needed for guru screen */
#include <pgserver/video.h>
#include <pgserver/font.h>
#include <pgserver/appmgr.h>
#include <stdio.h>
#include <stdarg.h>
#endif


g_error prerror(g_error e) {
  if (!iserror(e)) return e;
#ifndef TINY_MESSAGES
  printf("*** ERROR (");
  switch (errtype(e)) {
  case PG_ERRT_MEMORY: printf("MEMORY"); break;
  case PG_ERRT_IO: printf("IO"); break;
  case PG_ERRT_NETWORK: printf("NETWORK"); break;
  case PG_ERRT_BADPARAM: printf("BADPARAM"); break;
  case PG_ERRT_HANDLE: printf("HANDLE"); break;
  case PG_ERRT_INTERNAL: printf("INTERNAL"); break;
  case PG_ERRT_BUSY: printf("BUSY"); break;
  default: printf("?");
  }
  printf(") : %s\n",errortext(e));
#else
  puts(errortext(e));
#endif
  return e;
}

/* graphical error/info screen, only in debug mode */
#ifdef DEBUG_ANY

#define deadcomp_width 20
#define deadcomp_height 28
static char deadcomp_bits[] = {
  0xfe, 0xff, 0x07, 0x01, 0x00, 0x08, 0x01, 0x00, 0x08, 0xf1, 0xff, 0x08, 
  0x09, 0x00, 0x09, 0xa9, 0x50, 0x09, 0x49, 0x20, 0x09, 0xa9, 0x50, 0x09, 
  0x09, 0x00, 0x09, 0x09, 0x00, 0x09, 0x09, 0x00, 0x09, 0x89, 0x1f, 0x09, 
  0x49, 0x20, 0x09, 0x29, 0x40, 0x09, 0x09, 0x00, 0x09, 0xf1, 0xff, 0x08, 
  0x01, 0x00, 0x08, 0x01, 0x00, 0x08, 0x01, 0x00, 0x08, 0x01, 0x00, 0x08, 
  0x01, 0x00, 0x08, 0x01, 0x00, 0x08, 0x19, 0xff, 0x08, 0x01, 0x00, 0x08, 
  0x03, 0x00, 0x0c, 0x02, 0x00, 0x04, 0x02, 0x00, 0x04, 0xfe, 0xff, 0x07, 
  };

void guru(const char *fmt, ...) {
  struct fontdesc *df=NULL;
  hwrbitmap icon;
  char msgbuf[256];
  va_list ap;
  struct cliprect screenclip;
   
  if (!vid) return;

  /* Setup */
  (*vid->clear)();
  rdhandle((void**)&df,PG_TYPE_FONTDESC,-1,defaultfont);
  screenclip.x1 = screenclip.y1 = 0;
  screenclip.x2 = vid->xres-1;
  screenclip.y2 = vid->yres-1;
   
  /* Icon (if this fails, no big deal) */
  if (!iserror((*vid->bitmap_loadxbm)(&icon,deadcomp_bits,
				      deadcomp_width,deadcomp_height,
				      (*vid->color_pgtohwr)(0xFFFF80),
				      (*vid->color_pgtohwr)(0x000000)))) {
    (*vid->blit)(icon,0,0,5,5,deadcomp_width,deadcomp_height,PG_LGOP_NONE);
    (*vid->bitmap_free)(icon);
  }

  /* Format and print message */

  va_start(ap,fmt);
  vsnprintf(msgbuf,256,fmt,ap);
  va_end(ap);

  outtext(df,10+deadcomp_width,5,(*vid->color_pgtohwr)(0xFFFFFF),msgbuf,
	  &screenclip);
  (*vid->update)(0,0,vid->xres,vid->yres);
}

#endif /* DEBUG_ANY */

/* The End */

