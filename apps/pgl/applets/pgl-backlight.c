/* $Id: pgl-backlight.c,v 1.1 2001/08/11 23:16:35 micahjd Exp $
 * 
 * pgl-backlight.c - Very simple app to toggle backlight
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

#include <picogui.h>

unsigned char light_mask_bits[] = {
0x50, 0x34, 0x0A, 0x31, 0x33, 0x20, 0x31, 0x33, 0x0A, 0x02, 
0x00, 0x42, 0x10, 0x22, 0x20, 0x10, 0x40, 0x07, 0x00, 0x0F, 
0x80, 0xEF, 0xB8, 0x0F, 0x80, 0x07, 0x00, 0x10, 0x40, 0x22, 
0x20, 0x42, 0x10, 0x02, 0x00, 
};
#define light_mask_len 35

int main(int argc,char **argv) {
  pghandle wPGLbar,bLightMask,bLight;
  pgInit(argc,argv);

  /* Find the applet container */
  wPGLbar = pgFindWidget("PGL-AppletBar");
  if (!wPGLbar) {
    pgMessageDialog(argv[0],"This applet requires PGL",PG_MSGICON_ERROR);
    return 1;
  }
 
  /* Light mask is stored in PBM format */
  bLightMask = pgNewBitmap(pgFromMemory(light_mask_bits,light_mask_len));

  /* Light bitmap itself is mostly empty space, so draw it here */
  bLight = pgCreateBitmap(13,13);
  pgRender(bLight,PG_GROP_SETCOLOR,0x000000);
  pgRender(bLight,PG_GROP_RECT,0,0,13,13);
  pgRender(bLight,PG_GROP_SETCOLOR,0xFFFF00);
  pgRender(bLight,PG_GROP_RECT,5,5,3,3);

  /* Make our little toggle button */
  pgNewWidget(PG_WIDGET_FLATBUTTON,PG_DERIVE_INSIDE,wPGLbar);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_RIGHT,
	      PG_WP_BITMAP,bLight,
	      PG_WP_BITMASK,bLightMask,
	      PG_WP_EXTDEVENTS,PG_EXEV_TOGGLE,
	      0);
  //  pgBind(PGDEFAULT,PG_WE_ACTIVATE,&btnLight,NULL);

  pgEventLoop();
  return 0;
}

/* The End */




