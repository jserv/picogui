/* $Id: g_error.c,v 1.20 2001/03/17 04:16:34 micahjd Exp $
 *
 * g_error.h - Defines a format for errors
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
#ifdef CONFIG_TEXT
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

/* Without the XBM loader, no nifty icon! */
#ifdef CONFIG_FORMAT_XBM
#define deadcomp_width 20
#define deadcomp_height 28
char const deadcomp_bits[] = {
  0xfe, 0xff, 0x07, 0x01, 0x00, 0x08, 0x01, 0x00, 0x08, 0xf1, 0xff, 0x08, 
  0x09, 0x00, 0x09, 0xa9, 0x50, 0x09, 0x49, 0x20, 0x09, 0xa9, 0x50, 0x09, 
  0x09, 0x00, 0x09, 0x09, 0x00, 0x09, 0x09, 0x00, 0x09, 0x89, 0x1f, 0x09, 
  0x49, 0x20, 0x09, 0x29, 0x40, 0x09, 0x09, 0x00, 0x09, 0xf1, 0xff, 0x08, 
  0x01, 0x00, 0x08, 0x01, 0x00, 0x08, 0x01, 0x00, 0x08, 0x01, 0x00, 0x08, 
  0x01, 0x00, 0x08, 0x01, 0x00, 0x08, 0x19, 0xff, 0x08, 0x01, 0x00, 0x08, 
  0x03, 0x00, 0x0c, 0x02, 0x00, 0x04, 0x02, 0x00, 0x04, 0xfe, 0xff, 0x07, 
  };
#endif

void guru(const char *fmt, ...) {
  struct fontdesc *df=NULL;
  char msgbuf[256];  /* Cruftee! */
  va_list ap;
  struct cliprect screenclip;
   
  if (!vid) return;

  /* Setup */
  VID(clear) ();
  rdhandle((void**)&df,PG_TYPE_FONTDESC,-1,defaultfont);
  screenclip.x1 = screenclip.y1 = 0;
  screenclip.x2 = vid->xres-1;
  screenclip.y2 = vid->yres-1;

#ifdef CONFIG_FORMAT_XBM
  /* Icon (if this fails, no big deal) */
  {
     hwrbitmap icon;
     if (!iserror(VID(bitmap_loadxbm) (&icon,deadcomp_bits,
					 deadcomp_width,deadcomp_height,
					 VID(color_pgtohwr) (0xFFFF80),
					 VID(color_pgtohwr) (0x000000)))) {
	VID(blit) (icon,0,0,5,5,deadcomp_width,deadcomp_height,PG_LGOP_NONE);
	VID(bitmap_free) (icon);
     }
  }
#else
   /* To appease the below code */
# define deadcomp_width 0
#endif
     
  /* Format and print message */
  va_start(ap,fmt);
  vsnprintf(msgbuf,256,fmt,ap);
  va_end(ap);

  outtext(df,10+deadcomp_width,5,VID(color_pgtohwr) (0xFFFFFF),msgbuf,
	  &screenclip);
  VID(update) (0,0,vid->xres,vid->yres);
}

#endif /* DEBUG_ANY */

/* The End */

