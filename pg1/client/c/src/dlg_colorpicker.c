/* $Id$
 *
 * dlg_colorpicker.c - Implementation of the pColorPicker() function.
 *                     The current implementation is simple and only allows
 *                     selecting with RGB sliders. At some point I want to implement
 *                     a more complex color picker that can use various selection methods
 *                     depending on color depth and user preference.
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
 * 
 */

#include "clientlib.h"

/* Data for the color picker instance */
struct pickerdata {
  pghandle wOk, wCancel, wColorSample, wPicker;
  pghandle wInfo;
  pgcolor *c;
  pgcolor oldcolor;
};

void colorpicker_set(struct pickerdata *dat, pgcolor newcolor);
int colorpicker_truecolor(struct pickerdata *dat);
int pgColorPicker(pgcolor *c, const char *title);
int colorpicker_event(struct pickerdata *dat, struct pgEvent *evt);

/************************************************ Utility Functions */

/* Utility to set the currently chosen color and update the sample */
void colorpicker_set(struct pickerdata *dat, pgcolor newcolor) {
  *(dat->c) = newcolor; 
  pgWriteCmd(dat->wColorSample,PGCANVAS_SETGROP,1,newcolor);
  pgWriteCmd(dat->wColorSample,PGCANVAS_REDRAW,0);
  pgReplaceTextFmt(dat->wInfo,
		   "R: %d\nG: %d\nB: %d\n\n#%06X\n",
		   (newcolor>>16) & 0xFF,
		   (newcolor>>8) & 0xFF,
		   newcolor & 0xFF,
		   newcolor);
}

/* Handle widgets from the picker itself */
int colorpicker_event(struct pickerdata *dat, struct pgEvent *evt) {
  if (evt->from == dat->wOk)
    return 1;

  else if (evt->from == dat->wCancel) {
    *dat->c = dat->oldcolor;
    return 0;
  }
  
  return -1;
}

/************************************************ Color Picker functions */

/* Picker function for true color displays */
int colorpicker_truecolor(struct pickerdata *dat) {
  pghandle r,g,b;
  struct pgEvent evt;
  int ret;

  b = pgNewWidget(PG_WIDGET_SCROLL,PG_DERIVE_INSIDE,dat->wPicker);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIZE,255,
	      PG_WP_VALUE,255-(*dat->c & 0xFF),
	      0);
  pgNewWidget(PG_WIDGET_LABEL,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("B:"),
	      PG_WP_SIDE,PG_S_RIGHT,
	      0);
  g = pgNewWidget(PG_WIDGET_SCROLL,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIZE,255,
	      PG_WP_VALUE,255-((*dat->c >> 8) & 0xFF),
	      0);
  pgNewWidget(PG_WIDGET_LABEL,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("G:"),
	      PG_WP_SIDE,PG_S_RIGHT,
	      0);
  r = pgNewWidget(PG_WIDGET_SCROLL,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIZE,255,
	      PG_WP_VALUE,255-((*dat->c >> 16) & 0xFF),
	      0);
  pgNewWidget(PG_WIDGET_LABEL,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("R:"),
	      PG_WP_SIDE,PG_S_RIGHT,
	      0);

  for (;;) {
    evt = *pgGetEvent();

    if (evt.from == r) 
      colorpicker_set(dat,(*dat->c & 0x00FFFF) | ((255-evt.e.param) << 16));
    else if (evt.from == g) 
      colorpicker_set(dat,(*dat->c & 0xFF00FF) | ((255-evt.e.param) << 8));
    else if (evt.from == b) 
      colorpicker_set(dat,(*dat->c & 0xFFFF00) | (255-evt.e.param));

    else {
      ret = colorpicker_event(dat,&evt);
      if (ret!=-1)
	break;
    }
  }
  return ret;
}

/************************************************ Main Function */

/* This provides the shell common to all color pickers, and selects an
 * actual picker function based on the color depth.
 */
int pgColorPicker(pgcolor *c, const char *title) {
  pghandle wTB,wColorBox,wBox;
  struct pickerdata dat;
  int w,h,retval;
  pghandle fRGBinfo;
  dat.c = c;

  pgEnterContext();
  pgDialogBox(title);

  /* Create top-level widgets */

  wTB = pgNewWidget(PG_WIDGET_TOOLBAR,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_BOTTOM,
	      0);

  wColorBox = pgNewWidget(PG_WIDGET_BOX,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_RIGHT,
	      0);

  dat.wPicker = pgNewWidget(PG_WIDGET_BOX,PG_DERIVE_AFTER,wColorBox);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_ALL,
	      0);

  /* Toolbar buttons */

  dat.wOk = pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_INSIDE,wTB);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("Ok"),
	      PG_WP_SIDE,PG_S_RIGHT,
	      PG_WP_HOTKEY,PGKEY_RETURN,
	      PG_WP_IMAGE,pgThemeLookup(PGTH_O_POPUP_MESSAGEDLG,
					 PGTH_P_ICON_OK),
	      PG_WP_BITMASK,pgThemeLookup(PGTH_O_POPUP_MESSAGEDLG,
					  PGTH_P_ICON_OK_MASK),
	      0);

  dat.wCancel = pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("Cancel"),
	      PG_WP_SIDE,PG_S_RIGHT,
	      PG_WP_HOTKEY,PGKEY_ESCAPE,
	      PG_WP_IMAGE,pgThemeLookup(PGTH_O_POPUP_MESSAGEDLG,
					 PGTH_P_ICON_CANCEL),
	      PG_WP_BITMASK,pgThemeLookup(PGTH_O_POPUP_MESSAGEDLG,
					  PGTH_P_ICON_CANCEL_MASK),
	      0);

  /* Information and samples for the chosen color */

  fRGBinfo = pgNewFont(NULL,0,PG_FSTYLE_FIXED | PG_FSTYLE_DEFAULT);
  pgEnterContext();
  pgSizeText(&w,&h,fRGBinfo,pgNewString("0000000"));
  pgLeaveContext();

  dat.oldcolor = *c;

  /* original color */
  wBox = pgNewWidget(PG_WIDGET_BOX,PG_DERIVE_INSIDE,wColorBox);
  pgNewWidget(PG_WIDGET_CANVAS,PG_DERIVE_INSIDE,wBox);
  pgWriteCmd(0,PGCANVAS_GROP,2,PG_GROP_SETCOLOR,*c);
  pgWriteCmd(0,PGCANVAS_GROP,6,PG_GROP_SETMAPPING,0,0,w,h,PG_MAP_SCALE);
  pgWriteCmd(0,PGCANVAS_GROP,5,PG_GROP_RECT,0,0,w,h);
  pgWriteCmd(0,PGCANVAS_GROP,2,PG_GROP_SETCOLOR,0xFFFFFF);
  pgWriteCmd(0,PGCANVAS_GROP,2,PG_GROP_SETLGOP,PG_LGOP_XOR);
  pgWriteCmd(0,PGCANVAS_GROP,6,PG_GROP_TEXT,0,0,1,1,pgNewString("Old"));
  

  /* Selected color */
  wBox = pgNewWidget(PG_WIDGET_BOX,PG_DERIVE_AFTER,wBox);
  dat.wColorSample = pgNewWidget(PG_WIDGET_CANVAS,PG_DERIVE_INSIDE,wBox);
  pgWriteCmd(0,PGCANVAS_GROP,2,PG_GROP_SETCOLOR,*c);
  pgWriteCmd(0,PGCANVAS_GROP,6,PG_GROP_SETMAPPING,0,0,w,h,PG_MAP_SCALE);
  pgWriteCmd(0,PGCANVAS_GROP,5,PG_GROP_RECT,0,0,w,h);
  pgWriteCmd(0,PGCANVAS_GROP,2,PG_GROP_SETCOLOR,0xFFFFFF);
  pgWriteCmd(0,PGCANVAS_GROP,2,PG_GROP_SETLGOP,PG_LGOP_XOR);
  pgWriteCmd(0,PGCANVAS_GROP,6,PG_GROP_TEXT,0,0,1,1,pgNewString("New"));
  pgWriteCmd(0,PGCANVAS_FINDGROP,1,0);

  /* RGB info */
  dat.wInfo = pgNewWidget(PG_WIDGET_LABEL,PG_DERIVE_AFTER,wBox);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_ALL,
	      PG_WP_TRANSPARENT,0,
	      PG_WP_ALIGN,PG_A_LEFT,
	      PG_WP_FONT,fRGBinfo,
	      0);
  
  /* Display the initial color */
  colorpicker_set(&dat,*c);

  /* Run the color picker itself */
  retval = colorpicker_truecolor(&dat);

  pgLeaveContext();
  return retval;
}

/* The End */
